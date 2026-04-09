#include "Actor.hpp"
#include "Engine.hpp"
#include "Helper.h"
#include <glm/glm.hpp>
#include <string>

#include "ComponentManager.hpp"

// Getters
int          Actor::getId() const { return id; }
std::string  Actor::getName() { return name; }
SDL_Texture *Actor::getView() { return view; }
glm::vec2    Actor::getViewSize() { return view_size; }
glm::vec2    Actor::getPosition() const { return transform_position; }
glm::vec2    Actor::getVelocity() { return velocity; }
bool         Actor::getBlocking() { return blocking; }
bool         Actor::getTriggering() { return triggering; }
std::string  Actor::getNearbyDialogue() { return nearby_dialogue; }
std::string  Actor::getContactDialogue() { return contact_dialogue; }
std::string  Actor::getViewImage() { return view_image; }
std::string  Actor::getViewImageBack() { return view_image_back; }
std::string  Actor::getViewImageDamage() { return view_image_damage; }
std::string  Actor::getViewImageAttack() { return view_image_attack; }
glm::vec2    Actor::getTransformScale() { return transform_scale; }
float        Actor::getTransformRotationDegrees() { return transform_rotation_degrees; }
glm::vec2    Actor::getViewPivotOffset() { return view_pivot_offset; }
int          Actor::getRenderOrder() const { return renderOrder.value_or(transform_position.y); }
bool         Actor::getMovementBounce() { return movementBounce; }

bool Actor::operator==(const Actor &actor) const { return id == actor.id; }
bool Actor::operator<(const Actor &actor) const { return id < actor.id; }

bool Actor::getLastLooked() { return lastLooked; }
void Actor::setLastLooked(bool looked) { lastLooked = looked; }

void        Actor::setDamageSFXPath(std::string path) { damageSFXPath = path; }
void        Actor::setStepSFXPath(std::string path) { stepSFXPath = path; }
void        Actor::setDialogueSFXPath(std::string path) { dialogueSFXPath = path; }
std::string Actor::getDamageSFXPath() { return damageSFXPath; }
std::string Actor::getStepSFXPath() { return stepSFXPath; }
std::string Actor::getDialogueSFXPath() { return dialogueSFXPath; }

void Actor::setHasSpoken(bool spoken) { hasSpoken = spoken; }
bool Actor::getHasSpoken() { return hasSpoken; }

void Actor::injectConveninenceReferences(std::shared_ptr<luabridge::LuaRef> component) {
    (*component)["actor"] = this;
}

void Actor::markDestroyed() { destroyed = true; }
bool Actor::isDestroyed() const { return destroyed; }

void Actor::setDontDestroyOnLoad(bool value) { dontDestroyOnLoad = value; }
bool Actor::getDontDestroyOnLoad() const { return dontDestroyOnLoad; }

// Setters
void Actor::setId(int id) { this->id = id; }
void Actor::setName(std::string name) { this->name = name; }
void Actor::setView(SDL_Texture *view) { this->view = view; }
void Actor::setViewSize(glm::vec2 view_size) { this->view_size = view_size; }
void Actor::setPosition(glm::vec2 position) { this->transform_position = position; }
void Actor::setVelocity(glm::vec2 velocity) { this->velocity = velocity; }
void Actor::setBlocking(bool blocking) { this->blocking = blocking; }
void Actor::setTriggering(bool triggering) { this->triggering = triggering; }
void Actor::setNearbyDialogue(std::string nearby_dialogue) { this->nearby_dialogue = nearby_dialogue; }
void Actor::setContactDialogue(std::string contact_dialogue) { this->contact_dialogue = contact_dialogue; }
void Actor::setViewImage(std::string view_image) { this->view_image = view_image; }
void Actor::setViewImageBack(std::string view_image_back) { this->view_image_back = view_image_back; }
void Actor::setViewImageDamage(std::string view_image_damage) { this->view_image_damage = view_image_damage; }
void Actor::setViewImageAttack(std::string view_image_attack) { this->view_image_attack = view_image_attack; }
void Actor::setTransformScale(glm::vec2 transform_scale) { this->transform_scale = transform_scale; }
void Actor::setTransformRotationDegrees(float transform_rotation_degrees) { this->transform_rotation_degrees = transform_rotation_degrees; }
void Actor::setViewPivotOffset(glm::vec2 view_pivot_offset) { this->view_pivot_offset = view_pivot_offset; }
void Actor::setRenderOrder(int renderOrder) { this->renderOrder = renderOrder; }
void Actor::setMovementBounce(bool movement_bounce) { this->movementBounce = movement_bounce; }

void Actor::setXposition(float x) { this->transform_position.x = x; }
void Actor::setYposition(float y) { this->transform_position.y = y; }

void Actor::setXvelocity(float x) { this->velocity.x = x; }
void Actor::setYvelocity(float y) { this->velocity.y = y; }

void Actor::setTransformScaleX(float x) { this->transform_scale.x = x; }
void Actor::setTransformScaleY(float y) { this->transform_scale.y = y; }

void Actor::setViewPivotOffsetX(float x) { this->view_pivot_offset.x = x; }
void Actor::setViewPivotOffsetY(float y) { this->view_pivot_offset.y = y; }

// Other Functions
void Actor::move() { this->transform_position += this->velocity; }
void Actor::move(glm::vec2 newVelocity) { this->velocity = newVelocity; }
void Actor::reverseVelocity() { this->velocity *= -1; }

void Actor::swapActorView(std::string viewImage) {

    // Retrieve and set the new texture.
    SDL_Texture *newTex = Engine::getTextureCache()->at(viewImage);
    setView(newTex);

    float w, h;
    Helper::SDL_QueryTexture(newTex, &w, &h);

    setViewSize(glm::vec2(w, h));
}

std::unordered_map<std::string, std::shared_ptr<luabridge::LuaRef>> &Actor::getComponentsMap() {
    return components;
}

void Actor::addComponent(const std::string &name, std::shared_ptr<luabridge::LuaRef> component) {
    if (!component) {
        return;
    }

    auto existingIt = components.find(name);
    if (existingIt != components.end() && existingIt->second) {
        std::string existingType = (*existingIt->second)["type"].cast<std::string>();
        auto        typeIt       = componentKeysByType.find(existingType);
        if (typeIt != componentKeysByType.end()) {
            typeIt->second.erase(name);
            if (typeIt->second.empty()) {
                componentKeysByType.erase(typeIt);
            }
        }
    }

    components[name] = component;

    std::string typeName = (*component)["type"].cast<std::string>();
    componentKeysByType[typeName].insert(name);
}

std::shared_ptr<luabridge::LuaRef> Actor::getComponent(const std::string &name) {
    auto it = components.find(name);
    if (it == components.end()) {
        return nullptr;
    }
    return it->second;
}

void Actor::removeComponentByKey(const std::string &key) {
    auto componentIt = components.find(key);
    if (componentIt == components.end()) {
        return;
    }

    if (componentIt->second) {
        std::string typeName = (*(componentIt->second))["type"].cast<std::string>();
        auto        typeIt   = componentKeysByType.find(typeName);
        if (typeIt != componentKeysByType.end()) {
            typeIt->second.erase(key);
            if (typeIt->second.empty()) {
                componentKeysByType.erase(typeIt);
            }
        }
    }

    components.erase(componentIt);
}

luabridge::LuaRef Actor::AddComponent(std::string type_name) {
    lua_State *L = ComponentManager::getLuaState();

    luabridge::LuaRef inst = ComponentManager::InstantiateComponent(type_name);
    if (inst.isNil()) {
        return luabridge::LuaRef(L);
    }

    // 2) set built-ins.
    inst["type"]       = type_name;
    inst["enabled"]    = true;
    inst["hasStarted"] = false;

    // 3) generate global key
    static int  counter = 0;
    std::string key     = "r" + std::to_string(counter++);

    inst["key"] = key;

    // 4) inject convenience (actor pointer)
    inst["actor"] = this;

    // 5) schedule for *next* frame
    ComponentManager::scheduleRuntimeComponent(this, key, inst);

    // 6) return to Lua
    return inst;
}

luabridge::LuaRef Actor::getComponentByKey(std::string key) { //
    auto it = components.find(key);
    if (it != components.end()) {
        return *(it->second);
    }
    return luabridge::LuaRef(ComponentManager::getLuaState());
}

luabridge::LuaRef Actor::GetComponent(std::string typeName) {
    auto typeIt = componentKeysByType.find(typeName);
    if (typeIt == componentKeysByType.end() || typeIt->second.empty()) {
        return luabridge::LuaRef(ComponentManager::getLuaState());
    }

    const std::string &smallestKey = *(typeIt->second.begin());
    return *(components.at(smallestKey));
}

luabridge::LuaRef Actor::GetComponents(std::string typeName) {
    // Get the Lua state (assumes ComponentManager::getLuaState() returns it)
    lua_State *L = ComponentManager::getLuaState();
    // Create a new table
    luabridge::LuaRef resultTable = luabridge::newTable(L);
    auto              typeIt      = componentKeysByType.find(typeName);

    if (typeIt == componentKeysByType.end()) {
        return resultTable;
    }

    // Insert each matching component into the Lua table with Lua indexing (starting at 1).
    int index = 1;
    for (const auto &key : typeIt->second) {
        auto componentIt = components.find(key);
        if (componentIt == components.end()) {
            continue;
        }
        resultTable[index] = *(componentIt->second);
        index++;
    }

    return resultTable;
}

void Actor::RemoveComponent(const luabridge::LuaRef &component) {
    // grab its key
    std::string key = component["key"].cast<std::string>();

    // ask ComponentManager to run OnDestroy and erase it after LateUpdate.
    ComponentManager::scheduleComponentRemoval(this, key);
}