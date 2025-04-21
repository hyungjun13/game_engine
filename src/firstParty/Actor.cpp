#include "Actor.hpp"
#include "Engine.hpp"
#include "Helper.h"
#include <glm/glm.hpp>
#include <string>

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

luabridge::LuaRef Actor::addComponent(std::string type_name) {
    // Get the Lua state (make sure your ComponentManager provides access to it)
    lua_State *L = ComponentManager::getLuaState();

    // Create a new Lua table that will act as the component.
    luabridge::LuaRef component = luabridge::LuaRef::newTable(L);

    ComponentManager::EstablishInheritance(component, *ComponentManager::GetComponent(type_name));

    // Set the basic properties for the component.
    component["type"]       = type_name;
    component["enabled"]    = true;
    component["hasStarted"] = false;

    // Create a unique component key of the form "r<n>".
    // n is a global counter that increments each time a runtime component is added.
    static int  runtimeComponentCounter = 1;
    std::string compKey                 = "r" + std::to_string(runtimeComponentCounter++);

    // Insert the new component into the actor's component map.
    // (If you plan to use the "collect-then-alter" pattern later, consider inserting
    //  into a temporary container and merging at the end of the frame.)
    components[compKey] = std::make_shared<luabridge::LuaRef>(component);

    // Inject convenience references if necessary; for example,
    // you may wish to set the field "actor" in the component to point back to this actor.
    injectConveninenceReferences(components[compKey]);

    // Return the component.
    return component;
}