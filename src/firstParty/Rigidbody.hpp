#pragma once

#include "box2d/b2_body.h"
#include "box2d/b2_world.h"
#include <string>

class Actor;

struct CollisionInfo {
    Actor *other             = nullptr;
    b2Vec2 point             = b2Vec2(0.0f, 0.0f);
    b2Vec2 relative_velocity = b2Vec2(0.0f, 0.0f);
    b2Vec2 normal            = b2Vec2(0.0f, 0.0f);
};

class Rigidbody {
  public:
    Rigidbody()  = default;
    ~Rigidbody() = default;

    void OnStart();
    void OnUpdate() {}
    void OnLateUpdate() {}
    void OnDestroy();

    b2Vec2 GetPosition() const;
    float  GetRotation() const;
    b2Vec2 GetVelocity() const;
    float  GetAngularVelocity() const;

    b2Vec2 GetUpDirection() const;
    b2Vec2 GetRightDirection() const;

    void SetPosition(const b2Vec2 &position);
    void SetPositionX(float x_position);
    void SetPositionY(float y_position);
    void SetRotation(float rotation_degrees_clockwise);
    void SetVelocity(const b2Vec2 &velocity);
    void SetVelocityX(float x_velocity);
    void SetVelocityY(float y_velocity);
    void SetAngularVelocity(float angular_velocity_radians);
    void SetGravityScale(float new_gravity_scale);
    void SetUpDirection(const b2Vec2 &direction);
    void SetRightDirection(const b2Vec2 &direction);

    void AddForce(const b2Vec2 &force);

    static void     StepWorld();
    static bool     HasWorld();
    static b2World *GetWorld();

    std::string key  = "";
    std::string type = "Rigidbody";

    bool enabled    = true;
    bool hasStarted = false;

    Actor *actor = nullptr;

    // Suite #1 required properties.
    float       x                = 0.0f;
    float       y                = 0.0f;
    std::string body_type        = "dynamic";
    bool        precise          = true;
    float       gravity_scale    = 1.0f;
    float       density          = 1.0f;
    std::string collider_type    = "box";
    float       width            = 1.0f;
    float       height           = 1.0f;
    float       radius           = 0.5f;
    float       friction         = 0.3f;
    float       bounciness       = 0.3f;
    float       angular_friction = 0.3f;
    float       rotation         = 0.0f; // Clockwise degrees.
    bool        has_collider     = true;
    bool        has_trigger      = true;
    std::string trigger_type     = "box";
    float       trigger_width    = 1.0f;
    float       trigger_height   = 1.0f;
    float       trigger_radius   = 0.5f;

    // Backward-compatible aliases still used by existing scripts/projects.
    float velocityX = 0.0f;
    float velocityY = 0.0f;

    b2Body *body = nullptr;

  private:
    static b2World *EnsureWorld();
    static float    DegreesClockwiseToRadians(float rotation_degrees);
    static float    RadiansToDegreesClockwise(float rotation_radians);
    static float    ResolveDirectionAngleRadians(const b2Vec2 &direction, bool as_up_direction);

    static inline b2World *world = nullptr;
};
