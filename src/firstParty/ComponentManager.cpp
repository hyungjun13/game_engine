#include "ComponentManager.hpp"
#include "AudioDB.hpp"
#include "Engine.hpp"
#include "ImageDB.hpp"
#include "Input.hpp"
#include "Rigidbody.hpp"
#include "TextDB.hpp"
#include "box2d/b2_fixture.h"
#include "box2d/b2_math.h"
#include "box2d/b2_world_callbacks.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

#include "Helper.h"

namespace {
struct RaycastHit {
    Actor *actor      = nullptr;
    b2Vec2 point      = b2Vec2(0.0f, 0.0f);
    b2Vec2 normal     = b2Vec2(0.0f, 0.0f);
    bool   is_trigger = false;
    float  fraction   = 0.0f;
};

bool IsPhantomFixture(b2Fixture *fixture) {
    if (fixture == nullptr) {
        return true;
    }

    b2Filter filter = fixture->GetFilterData();
    return filter.maskBits == 0x0000;
}

Actor *ActorFromFixtureUserData(b2Fixture *fixture) {
    if (fixture == nullptr) {
        return nullptr;
    }

    uintptr_t pointer = fixture->GetUserData().pointer;
    if (pointer == 0) {
        return nullptr;
    }

    return reinterpret_cast<Actor *>(pointer);
}

luabridge::LuaRef MakeHitResultTable(lua_State *L, const RaycastHit &hit) {
    luabridge::LuaRef result = luabridge::newTable(L);
    result["actor"]          = hit.actor;
    result["point"]          = hit.point;
    result["normal"]         = hit.normal;
    result["is_trigger"]     = hit.is_trigger;
    return result;
}

class PhysicsRaycastClosestCallback : public b2RayCastCallback {
  public:
    float ReportFixture(b2Fixture    *fixture,
                        const b2Vec2 &point,
                        const b2Vec2 &normal,
                        float         fraction) override {
        if (IsPhantomFixture(fixture)) {
            return -1.0f;
        }

        Actor *actor = ActorFromFixtureUserData(fixture);
        if (actor == nullptr) {
            return -1.0f;
        }

        if (!has_hit_ || fraction < best_hit_.fraction) {
            best_hit_.actor      = actor;
            best_hit_.point      = point;
            best_hit_.normal     = normal;
            best_hit_.is_trigger = fixture->IsSensor();
            best_hit_.fraction   = fraction;
            has_hit_             = true;
        }

        return best_hit_.fraction;
    }

    bool HasHit() const {
        return has_hit_;
    }

    const RaycastHit &GetHit() const {
        return best_hit_;
    }

  private:
    RaycastHit best_hit_;
    bool       has_hit_ = false;
};

class PhysicsRaycastAllCallback : public b2RayCastCallback {
  public:
    float ReportFixture(b2Fixture    *fixture,
                        const b2Vec2 &point,
                        const b2Vec2 &normal,
                        float         fraction) override {
        if (IsPhantomFixture(fixture)) {
            return -1.0f;
        }

        Actor *actor = ActorFromFixtureUserData(fixture);
        if (actor == nullptr) {
            return -1.0f;
        }

        RaycastHit hit;
        hit.actor      = actor;
        hit.point      = point;
        hit.normal     = normal;
        hit.is_trigger = fixture->IsSensor();
        hit.fraction   = fraction;
        hits_.push_back(hit);

        return 1.0f;
    }

    const std::vector<RaycastHit> &GetHits() const {
        return hits_;
    }

  private:
    std::vector<RaycastHit> hits_;
};

luabridge::LuaRef PhysicsRaycast(const b2Vec2 &pos, const b2Vec2 &dir, float dist) {
    lua_State *L = ComponentManager::getLuaState();

    if (dist <= 0.0f || dir.LengthSquared() <= 0.0f || !Rigidbody::HasWorld()) {
        return luabridge::LuaRef(L);
    }

    b2World *world = Rigidbody::GetWorld();
    if (world == nullptr) {
        return luabridge::LuaRef(L);
    }

    b2Vec2 normalized_dir = dir;
    normalized_dir.Normalize();
    b2Vec2 ray_end = pos + dist * normalized_dir;

    PhysicsRaycastClosestCallback callback;
    world->RayCast(&callback, pos, ray_end);

    if (!callback.HasHit()) {
        return luabridge::LuaRef(L);
    }

    return MakeHitResultTable(L, callback.GetHit());
}

luabridge::LuaRef PhysicsRaycastAll(const b2Vec2 &pos, const b2Vec2 &dir, float dist) {
    lua_State        *L       = ComponentManager::getLuaState();
    luabridge::LuaRef results = luabridge::newTable(L);

    if (dist <= 0.0f || dir.LengthSquared() <= 0.0f || !Rigidbody::HasWorld()) {
        return results;
    }

    b2World *world = Rigidbody::GetWorld();
    if (world == nullptr) {
        return results;
    }

    b2Vec2 normalized_dir = dir;
    normalized_dir.Normalize();
    b2Vec2 ray_end = pos + dist * normalized_dir;

    PhysicsRaycastAllCallback callback;
    world->RayCast(&callback, pos, ray_end);

    std::vector<RaycastHit> hits = callback.GetHits();
    std::sort(hits.begin(), hits.end(), [](const RaycastHit &a, const RaycastHit &b) {
        return a.fraction < b.fraction;
    });

    int index = 1;
    for (const RaycastHit &hit : hits) {
        results[index] = MakeHitResultTable(L, hit);
        index++;
    }

    return results;
}

void CameraSetPosition(float x, float y) {
    Engine::setCameraPosition(glm::vec2(x, y));
}

glm::vec2 CameraGetPosition() {
    return Engine::getCameraPosition();
}

float CameraGetPositionX() {
    return Engine::getCameraPosition().x;
}

float CameraGetPositionY() {
    return Engine::getCameraPosition().y;
}

void CameraSetZoom(float zoom) {
    Engine::setZoomFactor(zoom);
}

float CameraGetZoom() {
    return Engine::getZoomFactor();
}

void CameraSetEase(float ease) {
    Engine::setCamEaseFactor(ease);
}

float CameraGetEase() {
    return Engine::getCamEaseFactor();
}

void SceneLoad(const std::string &sceneName) {
    Engine::LoadScene(sceneName);
}

std::string SceneGetCurrent() {
    return Engine::GetCurrentScene();
}

void SceneDontDestroy(Actor *actor) {
    Engine::DontDestroyActor(actor);
}

std::string MakeDestroyToken(Actor *actor, const std::string &key) {
    return std::to_string(reinterpret_cast<uintptr_t>(actor)) + ":" + key;
}

void DestroyRigidbodyBodyImmediatelyIfNeeded(const luabridge::LuaRef &instance) {
    if (!instance.isUserdata()) {
        return;
    }

    luabridge::LuaRef type_ref = instance["type"];
    if (!type_ref.isString() || type_ref.cast<std::string>() != "Rigidbody") {
        return;
    }

    try {
        Rigidbody *rb = instance.cast<Rigidbody *>();
        if (rb != nullptr) {
            rb->OnDestroy();
        }
    } catch (const luabridge::LuaException &) {
        return;
    }
}

bool LuaRefsRawEqual(const luabridge::LuaRef &a, const luabridge::LuaRef &b) {
    lua_State *L = ComponentManager::getLuaState();
    if (L == nullptr) {
        return false;
    }

    a.push(L);
    b.push(L);
    bool equal = lua_rawequal(L, -1, -2) != 0;
    lua_pop(L, 2);
    return equal;
}

std::string GetActorNameFromComponent(const luabridge::LuaRef &component) {
    if (!(component.isTable() || component.isUserdata())) {
        return "<event>";
    }

    luabridge::LuaRef actor_ref = component["actor"];
    if (!actor_ref.isUserdata()) {
        return "<event>";
    }

    try {
        Actor *actor = actor_ref.cast<Actor *>();
        if (actor != nullptr) {
            return actor->getName();
        }
    } catch (const luabridge::LuaException &) {
        return "<event>";
    }

    return "<event>";
}
} // namespace

void ComponentManager::Initialize() {
    InitializeState();
    InitializeFunctions();
    InitializeComponents();
}

bool ComponentManager::IsComponentLoaded(const std::string &componentName) {
    return loadedComponentCache.find(componentName) != loadedComponentCache.end();
}

std::shared_ptr<luabridge::LuaRef> ComponentManager::GetComponent(const std::string &componentName) {
    auto it = loadedComponentCache.find(componentName);
    if (it == loadedComponentCache.end()) {
        return nullptr;
    }
    return it->second;
}

bool ComponentManager::IsCppComponentType(const std::string &componentName) {
    return componentName == "Rigidbody";
}

bool ComponentManager::IsLuaComponentType(const std::string &componentName) {
    return !IsCppComponentType(componentName);
}

luabridge::LuaRef ComponentManager::CreateCppComponent(const std::string &componentName) {
    lua_State        *L           = getLuaState();
    luabridge::LuaRef constructor = luabridge::getGlobal(L, componentName.c_str());
    if (constructor.isNil()) {
        return luabridge::LuaRef(L);
    }

    // LuaBridge class registrations are callable via __call and are typically tables,
    // so do not require a strict function type check here.
    try {
        return constructor();
    } catch (const luabridge::LuaException &) {
        return luabridge::LuaRef(L);
    }
}

luabridge::LuaRef ComponentManager::CloneCppComponent(const luabridge::LuaRef &baseComponent) {
    lua_State *L = getLuaState();

    if (!baseComponent.isUserdata()) {
        return luabridge::LuaRef(L);
    }

    std::string       typeName = baseComponent["type"].cast<std::string>();
    luabridge::LuaRef clone    = CreateCppComponent(typeName);
    if (clone.isNil()) {
        return luabridge::LuaRef(L);
    }

    if (typeName == "Rigidbody") {
        auto *src = baseComponent.cast<Rigidbody *>();
        auto *dst = clone.cast<Rigidbody *>();
        if (src == nullptr || dst == nullptr) {
            return luabridge::LuaRef(L);
        }
        *dst = *src;
        return clone;
    }

    return luabridge::LuaRef(L);
}

std::shared_ptr<luabridge::LuaRef> ComponentManager::GetOrLoadComponentPrototype(const std::string &componentName) {
    auto existing = GetComponent(componentName);
    if (existing) {
        return existing;
    }

    lua_State *L = getLuaState();

    if (IsCppComponentType(componentName)) {
        luabridge::LuaRef instance = CreateCppComponent(componentName);
        if (instance.isNil()) {
            return nullptr;
        }

        auto compPtr = std::make_shared<luabridge::LuaRef>(instance);
        addComponentToCache(componentName, compPtr);
        return compPtr;
    }

    std::string luaComponentPath = "resources/component_types/" + componentName + ".lua";
    if (!std::filesystem::exists(luaComponentPath)) {
        return nullptr;
    }

    int loadStatus = luaL_dofile(L, luaComponentPath.c_str());
    if (loadStatus != LUA_OK) {
        std::cout << "problem with lua file " << componentName;
        exit(0);
    }

    luabridge::LuaRef component = luabridge::getGlobal(L, componentName.c_str());
    if (!component.isTable()) {
        std::cout << "error: component " << componentName << " is not a valid table";
        exit(0);
    }

    auto compPtr = std::make_shared<luabridge::LuaRef>(component);
    addComponentToCache(componentName, compPtr);
    return compPtr;
}

luabridge::LuaRef ComponentManager::InstantiateComponent(const std::string &componentName) {
    lua_State *L = getLuaState();

    auto baseComponent = GetOrLoadComponentPrototype(componentName);
    if (!baseComponent) {
        return luabridge::LuaRef(L);
    }

    if (baseComponent->isTable()) {
        luabridge::LuaRef instance = luabridge::newTable(L);
        EstablishInheritance(instance, *baseComponent);
        return instance;
    }

    if (baseComponent->isUserdata()) {
        return CloneCppComponent(*baseComponent);
    }

    return luabridge::LuaRef(L);
}

void ComponentManager::addComponentToCache(const std::string &componentName, std::shared_ptr<luabridge::LuaRef> component) {
    loadedComponentCache[componentName] = component;
}

void ComponentManager::setLuaState(lua_State *luaState) {
    L = luaState;
}

lua_State *ComponentManager::getLuaState() {
    return L;
}

void ComponentManager::ReportError(const std::string &actor_name, const luabridge::LuaException &e) {
    std::string error_message = e.what();

    std::replace(error_message.begin(), error_message.end(), '\\', '/');

    std::cout << "\033[31m" << actor_name << " : " << error_message << "\033[0m" << std::endl;
}

void ComponentManager::sortQueues() {
    for (auto &vec : onUpdateQueue) {
        std::sort(vec.begin(), vec.end(), [](const std::shared_ptr<luabridge::LuaRef> &a, const std::shared_ptr<luabridge::LuaRef> &b) {
            return (*a)["key"].cast<std::string>() < (*b)["key"].cast<std::string>();
        });
    }

    for (auto &vec : onLateUpdateQueue) {
        std::sort(vec.begin(), vec.end(), [](const std::shared_ptr<luabridge::LuaRef> &a, const std::shared_ptr<luabridge::LuaRef> &b) {
            return (*a)["key"].cast<std::string>() < (*b)["key"].cast<std::string>();
        });
    }
}

void ComponentManager::InitializeState() {
    setLuaState(luaL_newstate());
    luaL_openlibs(getLuaState());
}

void ComponentManager::InitializeFunctions() {
    lua_State *L = getLuaState();
    luabridge::getGlobalNamespace(L)
        .beginNamespace("Debug")
        .addFunction("Log", &ComponentManager::DebugLog)
        .endNamespace();

    // Existing glm::vec2 binding used by current camera/input APIs.
    luabridge::getGlobalNamespace(L)
        .beginClass<glm::vec2>("vec2")
        .addData("x", &glm::vec2::x)
        .addData("y", &glm::vec2::y)
        .endClass();

    // Suite 0 Vector2 API backed by Box2D's b2Vec2.
    luabridge::getGlobalNamespace(L)
        .beginClass<b2Vec2>("Vector2")
        .addConstructor<void (*)(float, float)>()
        .addData("x", &b2Vec2::x)
        .addData("y", &b2Vec2::y)
        .addFunction("__add", &b2Vec2::operator_add)
        .addFunction("__sub", &b2Vec2::operator_sub)
        .addFunction("__mul", &b2Vec2::operator_mul)
        .addStaticFunction("Dot", static_cast<float (*)(const b2Vec2 &, const b2Vec2 &)>(&b2Dot))
        .endClass();
    //
    luabridge::getGlobalNamespace(L)
        .beginClass<Actor>("Actor")
        .addFunction("GetID", &Actor::getId)
        .addFunction("GetName", &Actor::getName)
        .addFunction("GetComponentByKey", &Actor::getComponentByKey)
        .addFunction("GetComponent", &Actor::GetComponent)
        .addFunction("GetComponents", &Actor::GetComponents)
        .addFunction("AddComponent", &Actor::AddComponent)
        .addFunction("RemoveComponent", &Actor::RemoveComponent)
        .endClass();

    luabridge::getGlobalNamespace(L)
        .beginClass<CollisionInfo>("Collision")
        .addData("other", &CollisionInfo::other)
        .addData("point", &CollisionInfo::point)
        .addData("relative_velocity", &CollisionInfo::relative_velocity)
        .addData("normal", &CollisionInfo::normal)
        .endClass();

    luabridge::getGlobalNamespace(L)
        .beginClass<Rigidbody>("Rigidbody")
        .addConstructor<void (*)(void)>()
        .addFunction("OnStart", &Rigidbody::OnStart)
        .addFunction("OnUpdate", &Rigidbody::OnUpdate)
        .addFunction("OnLateUpdate", &Rigidbody::OnLateUpdate)
        .addFunction("OnDestroy", &Rigidbody::OnDestroy)
        .addFunction("GetPosition", &Rigidbody::GetPosition)
        .addFunction("GetRotation", &Rigidbody::GetRotation)
        .addFunction("GetVelocity", &Rigidbody::GetVelocity)
        .addFunction("GetAngularVelocity", &Rigidbody::GetAngularVelocity)
        .addFunction("GetUpDirection", &Rigidbody::GetUpDirection)
        .addFunction("GetRightDirection", &Rigidbody::GetRightDirection)
        .addFunction("SetPosition", &Rigidbody::SetPosition)
        .addFunction("SetPositionX", &Rigidbody::SetPositionX)
        .addFunction("SetPositionY", &Rigidbody::SetPositionY)
        .addFunction("SetRotation", &Rigidbody::SetRotation)
        .addFunction("SetVelocity", &Rigidbody::SetVelocity)
        .addFunction("SetVelocityX", &Rigidbody::SetVelocityX)
        .addFunction("SetVelocityY", &Rigidbody::SetVelocityY)
        .addFunction("SetAngularVelocity", &Rigidbody::SetAngularVelocity)
        .addFunction("SetGravityScale", &Rigidbody::SetGravityScale)
        .addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
        .addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
        .addFunction("AddForce", &Rigidbody::AddForce)
        .addData("key", &Rigidbody::key)
        .addData("type", &Rigidbody::type)
        .addData("enabled", &Rigidbody::enabled)
        .addData("hasStarted", &Rigidbody::hasStarted)
        .addData("actor", &Rigidbody::actor)
        // Suite #1 canonical property names.
        .addData("x", &Rigidbody::x)
        .addData("y", &Rigidbody::y)
        .addData("body_type", &Rigidbody::body_type)
        .addData("precise", &Rigidbody::precise)
        .addData("gravity_scale", &Rigidbody::gravity_scale)
        .addData("density", &Rigidbody::density)
        .addData("collider_type", &Rigidbody::collider_type)
        .addData("width", &Rigidbody::width)
        .addData("height", &Rigidbody::height)
        .addData("radius", &Rigidbody::radius)
        .addData("friction", &Rigidbody::friction)
        .addData("bounciness", &Rigidbody::bounciness)
        .addData("angular_friction", &Rigidbody::angular_friction)
        .addData("rotation", &Rigidbody::rotation)
        .addData("has_collider", &Rigidbody::has_collider)
        .addData("has_trigger", &Rigidbody::has_trigger)
        .addData("trigger_type", &Rigidbody::trigger_type)
        .addData("trigger_width", &Rigidbody::trigger_width)
        .addData("trigger_height", &Rigidbody::trigger_height)
        .addData("trigger_radius", &Rigidbody::trigger_radius)
        // Backward-compatible aliases.
        .addData("bodyType", &Rigidbody::body_type)
        .addData("bullet", &Rigidbody::precise)
        .addData("gravityScale", &Rigidbody::gravity_scale)
        .addData("angularDamping", &Rigidbody::angular_friction)
        .addData("colliderType", &Rigidbody::collider_type)
        .addData("collider_width", &Rigidbody::width)
        .addData("collider_height", &Rigidbody::height)
        .addData("restitution", &Rigidbody::bounciness)
        .addData("trigger", &Rigidbody::has_trigger)
        .addData("triggerType", &Rigidbody::trigger_type)
        .addData("velocityX", &Rigidbody::velocityX)
        .addData("velocityY", &Rigidbody::velocityY)
        .endClass();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Actor")
        .addFunction("Find", &Engine::Find)
        .addFunction("FindAll", &Engine::FindAll)
        .addFunction("Instantiate", &Engine::Instantiate)
        .addFunction("Destroy", &Engine::DestroyActor)
        .endNamespace();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Application")
        .addFunction("Quit", &ComponentManager::Quit)
        .addFunction("Sleep", &ComponentManager::Sleep)
        .addFunction("OpenURL", &ComponentManager::OpenURL)
        .addFunction("GetFrame", &ComponentManager::GetFrame)
        .endNamespace();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Input")
        .addFunction("GetKey", &Input::GetKey)
        .addFunction("GetKeyDown", &Input::GetKeyDown)
        .addFunction("GetKeyUp", &Input::GetKeyUp)
        .addFunction("GetMousePosition", &Input::GetMousePosition)
        .addFunction("GetMouseButton", &Input::GetMouseButton)
        .addFunction("GetMouseButtonDown", &Input::GetMouseButtonDown)
        .addFunction("GetMouseButtonUp", &Input::GetMouseButtonUp)
        .addFunction("GetMouseScrollDelta", &Input::GetMouseScrollDelta)
        .addFunction("HideCursor", &Input::HideCursor)
        .addFunction("ShowCursor", &Input::ShowCursor)
        .endNamespace();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Text")
        .addFunction("Draw", &TextDB::Draw)
        .endNamespace();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Image")
        .addFunction("Draw", &ImageDB::Draw)
        .addFunction("DrawEx", &ImageDB::Draw)
        .addFunction("DrawUI", &ImageDB::DrawUI)
        .addFunction("DrawUIEx", &ImageDB::DrawUIEx)
        .addFunction("DrawPixel", &ImageDB::DrawPixel)
        .endNamespace();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Audio")
        .addFunction("Play", &AudioDB::Play)
        .addFunction("Halt", &AudioDB::Halt)
        .addFunction("SetVolume", &AudioDB::SetVolume)
        .endNamespace();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Camera")
        .addFunction("SetPosition", &CameraSetPosition)
        .addFunction("GetPosition", &CameraGetPosition)
        .addFunction("GetPositionX", &CameraGetPositionX)
        .addFunction("GetPositionY", &CameraGetPositionY)
        .addFunction("SetZoom", &CameraSetZoom)
        .addFunction("GetZoom", &CameraGetZoom)
        .addFunction("SetZoomFactor", &CameraSetZoom)
        .addFunction("GetZoomFactor", &CameraGetZoom)
        .addFunction("SetEase", &CameraSetEase)
        .addFunction("GetEase", &CameraGetEase)
        .addFunction("SetCamEaseFactor", &CameraSetEase)
        .addFunction("GetCamEaseFactor", &CameraGetEase)
        .endNamespace();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Scene")
        .addFunction("Load", &SceneLoad)
        .addFunction("GetCurrent", &SceneGetCurrent)
        .addFunction("DontDestroy", &SceneDontDestroy)
        .endNamespace();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Physics")
        .addFunction("Raycast", &PhysicsRaycast)
        .addFunction("RaycastAll", &PhysicsRaycastAll)
        .endNamespace();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Event")
        .addFunction("Publish", &ComponentManager::EventPublish)
        .addFunction("Subscribe", &ComponentManager::EventSubscribe)
        .addFunction("Unsubscribe", &ComponentManager::EventUnsubscribe)
        .endNamespace();
}

void ComponentManager::InitializeComponents() {
    lua_State  *L            = getLuaState();
    std::string componentDir = "resources/component_types";

    // Register default native C++ component prototypes.
    {
        luabridge::LuaRef rigidbody = CreateCppComponent("Rigidbody");
        if (rigidbody.isNil()) {
            std::cout << "error: failed to construct native component Rigidbody";
            exit(0);
        }
        loadedComponentCache["Rigidbody"] = std::make_shared<luabridge::LuaRef>(rigidbody);
    }

    if (std::filesystem::exists(componentDir)) {
        for (const auto &entry : std::filesystem::directory_iterator(componentDir)) {
            if (entry.is_regular_file()) {
                std::filesystem::path filePath = entry.path();
                if (filePath.extension() == ".lua") {
                    int status = luaL_dofile(L, filePath.string().c_str());
                    if (status != LUA_OK) {
                        std::cout << "problem with lua file " << filePath.stem().string();
                        exit(0);
                    } else {
                        luabridge::LuaRef component = luabridge::getGlobal(L, filePath.stem().string().c_str());
                        if (!component.isTable()) {
                            std::cout << "error: component " << filePath.stem().string() << " is not a valid table";
                            exit(0);
                        } else {
                            loadedComponentCache[filePath.stem().string()] = std::make_shared<luabridge::LuaRef>(component);
                        }
                    }
                }
            }
        }
    }
}

int ComponentManager::DebugLog(lua_State *L) {
    const char *message = luaL_optstring(L, 1, "");
    std::cout << message << std::endl;
    return 0;
}

void ComponentManager::EstablishInheritance(luabridge::LuaRef &instance_table, luabridge::LuaRef &parent_table) {
    luabridge::LuaRef newMetatable = luabridge::newTable(getLuaState());
    newMetatable["__index"]        = parent_table;

    instance_table.push(getLuaState());
    newMetatable.push(getLuaState());
    lua_setmetatable(getLuaState(), -2);
    lua_pop(getLuaState(), 1);
}

// Now include the actorIndex so that components are queued per actor.
void ComponentManager::QueueOnStart(int actorIndex, const std::string &key, luabridge::LuaRef instance) {
    if (actorIndex < 0) {
        return;
    }
    const size_t actorIndexU = static_cast<size_t>(actorIndex);
    // Resize the vector if necessary.
    if (actorIndexU >= onStartQueue.size()) {
        onStartQueue.resize(actorIndexU + 1);
    }
    ComponentOnStartEntry entry = {key, instance};
    onStartQueue[actorIndexU].push(entry);
}

void ComponentManager::ProcessOnStart() {

    // Process each actor's PQ separately.
    for (auto &pq : onStartQueue) {
        while (!pq.empty()) {
            ComponentOnStartEntry entry = pq.top();
            pq.pop();
            if (entry.componentFunction["enabled"].cast<bool>() && !entry.componentFunction["hasStarted"].cast<bool>()) {
                try {

                    entry.componentFunction["OnStart"](entry.componentFunction);
                } catch (const luabridge::LuaException &e) {
                    // Report the error.
                    luabridge::LuaRef actorRef = entry.componentFunction["actor"];
                    Actor            *actorPtr = actorRef.cast<Actor *>();
                    ReportError(actorPtr->getName(), e);
                }
                // Set hasStarted to true after successful OnStart call.
                entry.componentFunction["hasStarted"] = true;
            }
        }
    }
    // Clear the vector after processing.
    onStartQueue.clear();
}

void ComponentManager::QueueOnUpdate(int actorIndex, std::shared_ptr<luabridge::LuaRef> instance) {
    if (actorIndex < 0) {
        return;
    }
    const size_t actorIndexU = static_cast<size_t>(actorIndex);
    // Resize the vector if necessary.
    if (actorIndexU >= onUpdateQueue.size()) {
        onUpdateQueue.resize(actorIndexU + 1);
    }
    onUpdateQueue[actorIndexU].push_back(instance);
    std::sort(onUpdateQueue[actorIndexU].begin(), onUpdateQueue[actorIndexU].end(),
              [](const std::shared_ptr<luabridge::LuaRef> &a, const std::shared_ptr<luabridge::LuaRef> &b) {
                  return (*a)["key"].cast<std::string>() < (*b)["key"].cast<std::string>();
              });
}

void ComponentManager::QueueOnLateUpdate(int actorIndex, std::shared_ptr<luabridge::LuaRef> instance) {
    if (actorIndex < 0) {
        return;
    }
    const size_t actorIndexU = static_cast<size_t>(actorIndex);
    // Resize the vector if necessary.
    if (actorIndexU >= onLateUpdateQueue.size()) {
        onLateUpdateQueue.resize(actorIndexU + 1);
    }
    onLateUpdateQueue[actorIndexU].push_back(instance);
    std::sort(onLateUpdateQueue[actorIndexU].begin(), onLateUpdateQueue[actorIndexU].end(),
              [](const std::shared_ptr<luabridge::LuaRef> &a, const std::shared_ptr<luabridge::LuaRef> &b) {
                  return (*a)["key"].cast<std::string>() < (*b)["key"].cast<std::string>();
              });
}
void ComponentManager::ProcessOnUpdate() {

    for (auto &actorQueue : onUpdateQueue) {
        for (auto &instance : actorQueue) {
            if ((*instance)["enabled"].cast<bool>()) {

                try {

                    (*instance)["OnUpdate"](*instance);

                } catch (const luabridge::LuaException &e) { //
                    // Report the error.
                    luabridge::LuaRef actorRef = (*instance)["actor"];
                    Actor            *a        = actorRef.cast<Actor *>();
                    ReportError(a->getName(), e);
                }
            }
        } //
    }
}

void ComponentManager::ProcessOnLateUpdate() {

    for (auto &actorQueue : onLateUpdateQueue) {

        for (auto &instance : actorQueue) {
            if ((*instance)["enabled"].cast<bool>()) {

                try {
                    // Call onLateUpdate and pass in actor instance as an argument.
                    (*instance)["OnLateUpdate"](*instance);

                } catch (const luabridge::LuaException &e) {
                    luabridge::LuaRef actorRef = (*instance)["actor"];
                    Actor            *a        = actorRef.cast<Actor *>();
                    ReportError(a->getName(), e);
                }
            }
        }
    }
}

void ComponentManager::Quit() {
    exit(0);
}

void ComponentManager::Sleep(int milliseconds) {
    // std::this_thread::sleep_for(std::chrono::milliseconds(dur_ms))

    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

int ComponentManager::GetFrame() {
    return Helper::GetFrameNumber();
}

void ComponentManager::OpenURL(const std::string &url) {
    /*The user’s Lua scripts must be able to call Application.OpenURL(url)
url is a string. The app will open the specified web page in the user’s default browser.
Hint : use std::system  paired with–
Windows : start <url> (chosen via _WIN32 preprocessor variable)
OSX : open <url> (chosen via __APPLE__ preprocessor variable)
Linux : xdg-open <url>
*/
#ifdef _WIN32
    std::string command = "start " + url;
#elif __APPLE__
    std::string command = "open " + url;
#elif __linux__
    std::string command = "xdg-open " + url;
#else
    return;

#endif
    std::system(command.c_str());
}

luabridge::LuaRef ComponentManager::GetComponentType(std::string typeName) {
    auto component = GetOrLoadComponentPrototype(typeName);
    if (!component) {
        return luabridge::LuaRef(getLuaState());
    }
    return *component;
}

void ComponentManager::scheduleRuntimeComponent(Actor            *a,
                                                std::string       key,
                                                luabridge::LuaRef inst) {
    pendingAdds.push_back({a, std::move(key), std::move(inst)});
}

void ComponentManager::flushPending() {
    for (auto &p : pendingAdds) {
        // actually insert into the actor’s map
        p.actor->addComponent(p.key, std::make_shared<luabridge::LuaRef>(p.instance));
        // queue their OnStart/OnUpdate/LateUpdate just like SceneLoader does:
        if (p.instance["OnStart"].isFunction())
            ComponentManager::QueueOnStart(
                p.actor->getId(), p.key, p.instance);
        if (p.instance["OnUpdate"].isFunction())
            ComponentManager::QueueOnUpdate(
                p.actor->getId(),
                std::make_shared<luabridge::LuaRef>(p.instance));
        if (p.instance["OnLateUpdate"].isFunction())
            ComponentManager::QueueOnLateUpdate(
                p.actor->getId(),
                std::make_shared<luabridge::LuaRef>(p.instance));
    }
    pendingAdds.clear();
}

void ComponentManager::ResetLifecycleQueues() {
    onStartQueue.clear();
    onUpdateQueue.clear();
    onLateUpdateQueue.clear();
    pendingAdds.clear();
    pendingDestroys.clear();
    pendingDestroyTokens.clear();
    pendingEventOps.clear();
}

void ComponentManager::EventPublish(const std::string &eventType, const luabridge::LuaRef &eventObject) {
    auto subscribersIt = eventSubscribers.find(eventType);
    if (subscribersIt == eventSubscribers.end()) {
        return;
    }

    for (const EventSubscription &subscription : subscribersIt->second) {
        if (!subscription.callback.isFunction()) {
            continue;
        }

        try {
            subscription.callback(subscription.component, eventObject);
        } catch (const luabridge::LuaException &e) {
            ReportError(GetActorNameFromComponent(subscription.component), e);
        }
    }
}

void ComponentManager::EventSubscribe(const std::string       &eventType,
                                      const luabridge::LuaRef &component,
                                      const luabridge::LuaRef &callback) {
    if (eventType.empty() || !(component.isTable() || component.isUserdata()) || !callback.isFunction()) {
        return;
    }

    pendingEventOps.push_back({PendingEventOpType::Subscribe, eventType, component, callback});
}

void ComponentManager::EventUnsubscribe(const std::string       &eventType,
                                        const luabridge::LuaRef &component,
                                        const luabridge::LuaRef &callback) {
    if (eventType.empty() || !(component.isTable() || component.isUserdata()) || !callback.isFunction()) {
        return;
    }

    pendingEventOps.push_back({PendingEventOpType::Unsubscribe, eventType, component, callback});
}

void ComponentManager::ProcessPendingEventSubscriptions() {
    if (pendingEventOps.empty()) {
        return;
    }

    for (const PendingEventOp &op : pendingEventOps) {
        if (op.type == PendingEventOpType::Subscribe) {
            eventSubscribers[op.eventType].push_back({op.component, op.callback, nextEventSequence++});
            continue;
        }

        auto subscribersIt = eventSubscribers.find(op.eventType);
        if (subscribersIt == eventSubscribers.end()) {
            continue;
        }

        auto &subscribers = subscribersIt->second;
        subscribers.erase(
            std::remove_if(subscribers.begin(), subscribers.end(), [&op](const EventSubscription &sub) {
                return LuaRefsRawEqual(sub.component, op.component) && LuaRefsRawEqual(sub.callback, op.callback);
            }),
            subscribers.end());

        if (subscribers.empty()) {
            eventSubscribers.erase(subscribersIt);
        }
    }

    pendingEventOps.clear();
}

void ComponentManager::scheduleComponentRemoval(Actor *actor, std::string key) {
    if (actor == nullptr) {
        return;
    }

    auto component = actor->getComponent(key);
    if (!component) {
        return;
    }

    std::string token = MakeDestroyToken(actor, key);
    if (pendingDestroyTokens.find(token) != pendingDestroyTokens.end()) {
        return;
    }

    (*component)["enabled"] = false;
    DestroyRigidbodyBodyImmediatelyIfNeeded(*component);

    pendingDestroyTokens.insert(token);
    pendingDestroys.push_back({actor, std::move(key), *component});
}

void ComponentManager::flushPendingRemovals() {
    std::sort(pendingDestroys.begin(), pendingDestroys.end(), [](const PendingComponent &a, const PendingComponent &b) {
        if (a.actor != b.actor) {
            return a.actor < b.actor;
        }
        return a.key < b.key;
    });

    for (auto &entry : pendingDestroys) {
        if (entry.actor == nullptr) {
            continue;
        }

        luabridge::LuaRef on_destroy = entry.instance["OnDestroy"];
        if (!on_destroy.isFunction()) {
            continue;
        }

        try {
            on_destroy(entry.instance);
        } catch (const luabridge::LuaException &e) {
            ComponentManager::ReportError(entry.actor->getName(), e);
        }
    }

    for (auto &entry : pendingDestroys) {
        if (entry.actor == nullptr) {
            continue;
        }
        entry.actor->removeComponentByKey(entry.key);
        removeEventSubscriptionsForComponent(entry.instance);
    }

    pendingDestroys.clear();
    pendingDestroyTokens.clear();
}

void ComponentManager::removeEventSubscriptionsForComponent(const luabridge::LuaRef &component) {
    for (auto it = eventSubscribers.begin(); it != eventSubscribers.end();) {
        auto &subscribers = it->second;

        subscribers.erase(
            std::remove_if(subscribers.begin(), subscribers.end(), [&component](const EventSubscription &sub) {
                return LuaRefsRawEqual(sub.component, component);
            }),
            subscribers.end());

        if (subscribers.empty()) {
            it = eventSubscribers.erase(it);
        } else {
            ++it;
        }
    }

    pendingEventOps.erase(
        std::remove_if(pendingEventOps.begin(), pendingEventOps.end(), [&component](const PendingEventOp &op) {
            return LuaRefsRawEqual(op.component, component);
        }),
        pendingEventOps.end());
}

void ComponentManager::ProcessOnDestroy() {
    if (pendingDestroys.empty()) {
        return;
    }
    flushPendingRemovals();
}