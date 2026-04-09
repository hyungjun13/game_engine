#include "Rigidbody.hpp"

#include "Actor.hpp"
#include "ComponentManager.hpp"

#include "box2d/b2_circle_shape.h"
#include "box2d/b2_contact.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_world_callbacks.h"

#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>

#include <algorithm>
#include <cctype>
#include <utility>
#include <vector>

namespace {
b2BodyType ResolveBodyType(const std::string &body_type) {
    if (body_type == "static") {
        return b2_staticBody;
    }
    if (body_type == "kinematic") {
        return b2_kinematicBody;
    }
    return b2_dynamicBody;
}

float SafeHalfExtent(float extent) {
    if (extent <= 0.0f) {
        return 0.5f;
    }
    return extent * 0.5f;
}

float SafeRadius(float radius) {
    if (radius <= 0.0f) {
        return 0.5f;
    }
    return radius;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

constexpr float    SUITE4_INVALID_COLLISION_VALUE = -999.0f;
constexpr uint16_t COLLIDER_CATEGORY_BITS         = 0x0001;
constexpr uint16_t TRIGGER_CATEGORY_BITS          = 0x0002;
constexpr uint16_t PHANTOM_CATEGORY_BITS          = 0x0004;

enum class FixtureKind {
    Collider,
    Trigger,
    Phantom,
};

Actor *ActorFromFixture(b2Fixture *fixture) {
    if (fixture == nullptr) {
        return nullptr;
    }

    uintptr_t pointer = fixture->GetUserData().pointer;
    if (pointer == 0) {
        return nullptr;
    }

    return reinterpret_cast<Actor *>(pointer);
}

CollisionInfo BuildCollisionInfo(b2Contact *contact,
                                 b2Fixture *fixture_a,
                                 b2Fixture *fixture_b,
                                 Actor     *other_actor,
                                 bool       use_sentinel_point_and_normal) {
    CollisionInfo collision;
    collision.other = other_actor;

    if (fixture_a != nullptr && fixture_b != nullptr) {
        collision.relative_velocity = fixture_a->GetBody()->GetLinearVelocity() - fixture_b->GetBody()->GetLinearVelocity();
    }

    if (use_sentinel_point_and_normal) {
        collision.point  = b2Vec2(SUITE4_INVALID_COLLISION_VALUE, SUITE4_INVALID_COLLISION_VALUE);
        collision.normal = b2Vec2(SUITE4_INVALID_COLLISION_VALUE, SUITE4_INVALID_COLLISION_VALUE);
        return collision;
    }

    b2WorldManifold world_manifold;
    contact->GetWorldManifold(&world_manifold);
    collision.point  = world_manifold.points[0];
    collision.normal = world_manifold.normal;

    return collision;
}

void InvokeCollisionLifecycle(Actor               *actor,
                              const char          *lifecycle_name,
                              const CollisionInfo &collision) {
    if (actor == nullptr) {
        return;
    }

    auto                                                                   &components_map = actor->getComponentsMap();
    std::vector<std::pair<std::string, std::shared_ptr<luabridge::LuaRef>>> sorted_components;
    sorted_components.reserve(components_map.size());
    for (const auto &entry : components_map) {
        sorted_components.push_back(entry);
    }

    std::sort(sorted_components.begin(), sorted_components.end(), [](const auto &a, const auto &b) {
        return a.first < b.first;
    });

    for (const auto &entry : sorted_components) {
        const auto &component = entry.second;
        if (!component) {
            continue;
        }

        luabridge::LuaRef enabled_ref = (*component)["enabled"];
        if (enabled_ref.isBool() && !enabled_ref.cast<bool>()) {
            continue;
        }

        luabridge::LuaRef lifecycle_ref = (*component)[lifecycle_name];
        if (!lifecycle_ref.isFunction()) {
            continue;
        }

        try {
            lifecycle_ref(*component, collision);
        } catch (const luabridge::LuaException &e) {
            ComponentManager::ReportError(actor->getName(), e);
        }
    }
}

class RigidbodyContactListener : public b2ContactListener {
  public:
    void BeginContact(b2Contact *contact) override {
        Dispatch(contact, false);
    }

    void EndContact(b2Contact *contact) override {
        Dispatch(contact, true);
    }

  private:
    void Dispatch(b2Contact *contact, bool is_exit_event) {
        if (contact == nullptr) {
            return;
        }

        b2Fixture *fixture_a = contact->GetFixtureA();
        b2Fixture *fixture_b = contact->GetFixtureB();

        Actor *actor_a = ActorFromFixture(fixture_a);
        Actor *actor_b = ActorFromFixture(fixture_b);

        bool sensors_contact = fixture_a->IsSensor() && fixture_b->IsSensor();
        bool solids_contact  = !fixture_a->IsSensor() && !fixture_b->IsSensor();

        const char *lifecycle_name = nullptr;
        bool        use_sentinel   = false;

        if (sensors_contact) {
            lifecycle_name = is_exit_event ? "OnTriggerExit" : "OnTriggerEnter";
            use_sentinel   = true;
        } else if (solids_contact) {
            lifecycle_name = is_exit_event ? "OnCollisionExit" : "OnCollisionEnter";
            use_sentinel   = is_exit_event;
        } else {
            return;
        }

        CollisionInfo collision_a = BuildCollisionInfo(contact, fixture_a, fixture_b, actor_b, use_sentinel);
        InvokeCollisionLifecycle(actor_a, lifecycle_name, collision_a);

        CollisionInfo collision_b = BuildCollisionInfo(contact, fixture_a, fixture_b, actor_a, use_sentinel);
        InvokeCollisionLifecycle(actor_b, lifecycle_name, collision_b);
    }
};

RigidbodyContactListener g_rigidbody_contact_listener;

void CreateFixture(b2Body            *body,
                   Actor             *owner_actor,
                   const std::string &shape_type,
                   float              width,
                   float              height,
                   float              radius,
                   float              density,
                   float              friction,
                   float              bounciness,
                   FixtureKind        fixture_kind) {
    std::string type_lower = ToLower(shape_type);

    b2PolygonShape box_shape;
    b2CircleShape  circle_shape;

    if (type_lower == "circle") {
        circle_shape.m_radius = SafeRadius(radius);
    } else {
        box_shape.SetAsBox(SafeHalfExtent(width), SafeHalfExtent(height));
    }

    b2FixtureDef fixture_def;
    fixture_def.shape            = (type_lower == "circle") ? static_cast<b2Shape *>(&circle_shape) : static_cast<b2Shape *>(&box_shape);
    fixture_def.density          = density;
    fixture_def.friction         = friction;
    fixture_def.restitution      = bounciness;
    fixture_def.isSensor         = (fixture_kind != FixtureKind::Collider);
    fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(owner_actor);

    if (fixture_kind == FixtureKind::Collider) {
        fixture_def.filter.categoryBits = COLLIDER_CATEGORY_BITS;
        fixture_def.filter.maskBits     = COLLIDER_CATEGORY_BITS;
    } else if (fixture_kind == FixtureKind::Trigger) {
        fixture_def.filter.categoryBits = TRIGGER_CATEGORY_BITS;
        fixture_def.filter.maskBits     = TRIGGER_CATEGORY_BITS;
    } else {
        fixture_def.filter.categoryBits = PHANTOM_CATEGORY_BITS;
        fixture_def.filter.maskBits     = 0x0000;
    }

    body->CreateFixture(&fixture_def);
}
} // namespace

b2World *Rigidbody::EnsureWorld() {
    if (world == nullptr) {
        world = new b2World(b2Vec2(0.0f, 9.8f));
        world->SetContactListener(&g_rigidbody_contact_listener);
    }
    return world;
}

float Rigidbody::DegreesClockwiseToRadians(float rotation_degrees) {
    float radians = rotation_degrees * (b2_pi / 180.0f);
    return -radians;
}

float Rigidbody::RadiansToDegreesClockwise(float rotation_radians) {
    float degrees = rotation_radians * (180.0f / b2_pi);
    return -degrees;
}

float Rigidbody::ResolveDirectionAngleRadians(const b2Vec2 &direction, bool as_up_direction) {
    b2Vec2 normalized_direction = direction;
    if (normalized_direction.LengthSquared() <= 0.0f) {
        return 0.0f;
    }
    normalized_direction.Normalize();

    if (as_up_direction) {
        return glm::atan(normalized_direction.x, -normalized_direction.y);
    }
    return glm::atan(normalized_direction.y, normalized_direction.x);
}

void Rigidbody::OnStart() {
    if (body != nullptr) {
        return;
    }

    b2World *physics_world = EnsureWorld();

    b2BodyDef body_def;
    body_def.type           = ResolveBodyType(body_type);
    body_def.bullet         = precise;
    body_def.gravityScale   = gravity_scale;
    body_def.angularDamping = angular_friction;
    body_def.angle          = DegreesClockwiseToRadians(rotation);
    body_def.linearVelocity.Set(velocityX, velocityY);

    // Preserve current projects where actor position is the setup source unless
    // Rigidbody x/y are explicitly provided.
    float spawn_x = x;
    float spawn_y = y;
    if (actor != nullptr) {
        glm::vec2 actor_position = actor->getPosition();
        if ((x == 0.0f && y == 0.0f) && (actor_position.x != 0.0f || actor_position.y != 0.0f)) {
            spawn_x = actor_position.x;
            spawn_y = actor_position.y;
        }
    }

    body_def.position.Set(spawn_x, spawn_y);

    body                        = physics_world->CreateBody(&body_def);
    body->GetUserData().pointer = reinterpret_cast<uintptr_t>(this);

    if (has_collider) {
        CreateFixture(body, actor, collider_type, width, height, radius, density, friction, bounciness, FixtureKind::Collider);
    }

    if (has_trigger) {
        float trigger_density = has_collider ? 0.0f : density;
        CreateFixture(body,
                      actor,
                      trigger_type,
                      trigger_width,
                      trigger_height,
                      trigger_radius,
                      trigger_density,
                      friction,
                      bounciness,
                      FixtureKind::Trigger);
    }

    // Suite 2 behavior: when neither collider nor trigger is present,
    // create a phantom sensor fixture so dynamic bodies still simulate.
    if (!has_collider && !has_trigger) {
        CreateFixture(body, actor, collider_type, width, height, radius, density, friction, bounciness, FixtureKind::Phantom);
    }

    x = spawn_x;
    y = spawn_y;

    if (actor != nullptr) {
        actor->setPosition(glm::vec2(spawn_x, spawn_y));
        actor->setTransformRotationDegrees(rotation);
    }
}

void Rigidbody::OnDestroy() {
    if (body == nullptr) {
        return;
    }

    if (world != nullptr) {
        world->DestroyBody(body);
    }
    body = nullptr;
}

b2Vec2 Rigidbody::GetPosition() const {
    if (body != nullptr) {
        return body->GetPosition();
    }
    return b2Vec2(x, y);
}

float Rigidbody::GetRotation() const {
    if (body != nullptr) {
        return RadiansToDegreesClockwise(body->GetAngle());
    }
    return rotation;
}

b2Vec2 Rigidbody::GetVelocity() const {
    if (body != nullptr) {
        return body->GetLinearVelocity();
    }
    return b2Vec2(velocityX, velocityY);
}

float Rigidbody::GetAngularVelocity() const {
    if (body != nullptr) {
        return body->GetAngularVelocity();
    }
    return 0.0f;
}

b2Vec2 Rigidbody::GetUpDirection() const {
    float angle = 0.0f;
    if (body != nullptr) {
        angle = body->GetAngle();
    } else {
        angle = DegreesClockwiseToRadians(rotation);
    }

    b2Vec2 result = b2Vec2(std::sin(angle), -std::cos(angle));
    result.Normalize();
    return result;
}

b2Vec2 Rigidbody::GetRightDirection() const {
    float angle = 0.0f;
    if (body != nullptr) {
        angle = body->GetAngle();
    } else {
        angle = DegreesClockwiseToRadians(rotation);
    }

    b2Vec2 result = b2Vec2(std::cos(angle), std::sin(angle));
    result.Normalize();
    return result;
}

void Rigidbody::SetPosition(const b2Vec2 &position) {
    x = position.x;
    y = position.y;

    if (body != nullptr) {
        body->SetTransform(position, body->GetAngle());
    }

    if (actor != nullptr) {
        actor->setPosition(glm::vec2(position.x, position.y));
    }
}

void Rigidbody::SetPositionX(float x_position) {
    b2Vec2 current = GetPosition();
    SetPosition(b2Vec2(x_position, current.y));
}

void Rigidbody::SetPositionY(float y_position) {
    b2Vec2 current = GetPosition();
    SetPosition(b2Vec2(current.x, y_position));
}

void Rigidbody::SetRotation(float rotation_degrees_clockwise) {
    rotation                = rotation_degrees_clockwise;
    float new_angle_radians = DegreesClockwiseToRadians(rotation_degrees_clockwise);

    if (body != nullptr) {
        body->SetTransform(body->GetPosition(), new_angle_radians);
    }

    if (actor != nullptr) {
        actor->setTransformRotationDegrees(rotation_degrees_clockwise);
    }
}

void Rigidbody::SetVelocity(const b2Vec2 &velocity) {
    velocityX = velocity.x;
    velocityY = velocity.y;

    if (body != nullptr) {
        body->SetLinearVelocity(velocity);
    }
}

void Rigidbody::SetVelocityX(float x_velocity) {
    b2Vec2 current_velocity = GetVelocity();
    SetVelocity(b2Vec2(x_velocity, current_velocity.y));
}

void Rigidbody::SetVelocityY(float y_velocity) {
    b2Vec2 current_velocity = GetVelocity();
    SetVelocity(b2Vec2(current_velocity.x, y_velocity));
}

void Rigidbody::SetAngularVelocity(float angular_velocity_radians) {
    if (body != nullptr) {
        body->SetAngularVelocity(angular_velocity_radians);
    }
}

void Rigidbody::SetGravityScale(float new_gravity_scale) {
    gravity_scale = new_gravity_scale;
    if (body != nullptr) {
        body->SetGravityScale(new_gravity_scale);
    }
}

void Rigidbody::SetUpDirection(const b2Vec2 &direction) {
    if (direction.LengthSquared() <= 0.0f) {
        return;
    }

    float new_angle_radians = ResolveDirectionAngleRadians(direction, true);
    rotation                = RadiansToDegreesClockwise(new_angle_radians);

    if (body != nullptr) {
        body->SetTransform(body->GetPosition(), new_angle_radians);
    }

    if (actor != nullptr) {
        actor->setTransformRotationDegrees(rotation);
    }
}

void Rigidbody::SetRightDirection(const b2Vec2 &direction) {
    if (direction.LengthSquared() <= 0.0f) {
        return;
    }

    float new_angle_radians = ResolveDirectionAngleRadians(direction, false);
    rotation                = RadiansToDegreesClockwise(new_angle_radians);

    if (body != nullptr) {
        body->SetTransform(body->GetPosition(), new_angle_radians);
    }

    if (actor != nullptr) {
        actor->setTransformRotationDegrees(rotation);
    }
}

void Rigidbody::AddForce(const b2Vec2 &force) {
    if (body == nullptr) {
        return;
    }
    body->ApplyForceToCenter(force, true);
}

void Rigidbody::StepWorld() {
    if (world == nullptr) {
        return;
    }

    world->Step(1.0f / 60.0f, 8, 3);
}

bool Rigidbody::HasWorld() {
    return world != nullptr;
}

b2World *Rigidbody::GetWorld() {
    return world;
}
