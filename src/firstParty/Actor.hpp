#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

#include "SDL2/SDL.h"

#include "lua.hpp"

#include "LuaBridge.h"

#include "ComponentManager.hpp"
//
// #include "Engine.hpp"

class Actor {
  public:
    // Getters
    int          getId() const;
    std::string  getName();
    SDL_Texture *getView();
    glm::vec2    getViewSize();
    glm::vec2    getPosition() const;
    glm::vec2    getVelocity();
    bool         getBlocking();
    bool         getTriggering();
    std::string  getNearbyDialogue();
    std::string  getContactDialogue();
    std::string  getViewImage();
    std::string  getViewImageBack();
    std::string  getViewImageDamage();
    std::string  getViewImageAttack();
    glm::vec2    getTransformScale();
    float        getTransformRotationDegrees();
    glm::vec2    getViewPivotOffset();
    bool         getMovementBounce();
    int          getRenderOrder() const;
    glm::vec2    getBoxCollider();
    float        getBoxColliderX();
    float        getBoxColliderY();
    glm::vec2    getBoxTrigger();
    float        getBoxTriggerX();
    float        getBoxTriggerY();

    // Setters
    void setId(int id);
    void setName(std::string name);
    void setView(SDL_Texture *view);
    void setViewSize(glm::vec2 view_size);
    void setPosition(glm::vec2 position);
    void setVelocity(glm::vec2 velocity);
    void setBlocking(bool blocking);
    void setTriggering(bool triggering);
    void setNearbyDialogue(std::string nearby_dialogue);
    void setContactDialogue(std::string contact_dialogue);
    void setViewImage(std::string view_image);
    void setViewImageBack(std::string view_image_back);
    void setViewImageDamage(std::string view_image_damage);
    void setViewImageAttack(std::string view_image_attack);
    void setTransformScale(glm::vec2 transform_scale);
    void setTransformRotationDegrees(float transform_rotation_degrees);
    void setViewPivotOffset(glm::vec2 view_pivot_offset);
    void setRenderOrder(int renderOrder);
    void setMovementBounce(bool movementBounce);

    void move();
    void move(glm::vec2 newVelocity);
    void reverseVelocity();

    void setXposition(float x);
    void setYposition(float y);
    void setXvelocity(float x);
    void setYvelocity(float y);
    void setTransformScaleX(float x);
    void setTransformScaleY(float y);
    void setViewPivotOffsetX(float x);
    void setViewPivotOffsetY(float y);

    // Add comparator
    bool
    operator==(const Actor &actor) const {
        return id == actor.id;
    }

    bool operator<(const Actor &actor) const {
        return id < actor.id;
    }

    bool getLastLooked() { return lastLooked; }
    void setLastLooked(bool looked) { lastLooked = looked; }

    void swapActorView(std::string viewImage);

    void        setDamageSFXPath(std::string path) { damageSFXPath = path; }
    void        setStepSFXPath(std::string path) { stepSFXPath = path; }
    void        setDialogueSFXPath(std::string path) { dialogueSFXPath = path; }
    std::string getDamageSFXPath() { return damageSFXPath; }
    std::string getStepSFXPath() { return stepSFXPath; }
    std::string getDialogueSFXPath() { return dialogueSFXPath; }

    void setHasSpoken(bool hasSpoken) { this->hasSpoken = hasSpoken; }
    bool getHasSpoken() { return hasSpoken; }

    void addComponent(std::string name, std::shared_ptr<luabridge::LuaRef> component) {
        components[name] = component;
    }

    std::shared_ptr<luabridge::LuaRef> getComponent(std::string name) {
        return components[name];
    }
    std::unordered_map<std::string, std::shared_ptr<luabridge::LuaRef>> &getComponentsMap();

    void injectConveninenceReferences(std::shared_ptr<luabridge::LuaRef> component) {
        (*component)["actor"] = this;
    }

    luabridge::LuaRef getComponentByKey(std::string key) { //
        if (components.find(key) != components.end()) {
            return *(components[key]);
        } else {
            return luabridge::LuaRef(ComponentManager::getLuaState());
        }
    }

    luabridge::LuaRef GetComponent(std::string typeName) {
        std::vector<std::string> matchingKeys;
        for (const auto &pair : components) {
            // Check if the component's "type" field equals typeName.
            if (((*pair.second)["type"]).cast<std::string>() == typeName) {
                matchingKeys.push_back(pair.first);
            }
        }

        // If no matching components found, return nil.
        if (matchingKeys.empty()) {
            return luabridge::LuaRef(ComponentManager::getLuaState());
        }

        // Sort the keys lexicographically.
        std::sort(matchingKeys.begin(), matchingKeys.end());

        // Return the component corresponding to the lexicographically smallest key.
        return *(components.at(matchingKeys[0]));
    }

    luabridge::LuaRef GetComponents(std::string typeName) {
        // Get the Lua state (assumes ComponentManager::getLuaState() returns it)
        lua_State *L = ComponentManager::getLuaState();
        // Create a new table
        luabridge::LuaRef        resultTable = luabridge::newTable(L);
        std::vector<std::string> matchingKeys;

        // Collect keys for components with a matching "type" field .
        for (const auto &pair : components) {
            // Make sure the "type" field exists and cast it to string.
            // This works if you explicitly set instance["type"] when creating the component.
            if (((*pair.second)["type"]).cast<std::string>() == typeName) {
                matchingKeys.push_back(pair.first);
            }
        }

        // Sort the keys lexicographically.
        std::sort(matchingKeys.begin(), matchingKeys.end());

        // Insert each matching component into the Lua table with Lua indexing (starting at 1).
        int index = 1;
        for (const auto &key : matchingKeys) {
            resultTable[index] = *components[key];
            index++;
        }

        // If no components matched, the table will be empty.
        return resultTable;
    }

    luabridge::LuaRef addComponent(std::string type_name);

  private:
    int                id                         = -1;
    std::string        name                       = "";
    SDL_Texture       *view                       = nullptr;
    glm::vec2          view_size                  = glm::vec2(0.0f, 0.0f);
    glm::vec2          transform_position         = glm::vec2(0, 0);
    glm::vec2          velocity                   = glm::vec2(0, 0);
    bool               blocking                   = false;
    bool               triggering                 = false;
    std::string        nearby_dialogue            = "";
    std::string        contact_dialogue           = "";
    std::string        view_image                 = "";
    std::string        view_image_back            = "";
    std::string        view_image_damage          = "";
    std::string        view_image_attack          = "";
    glm::vec2          transform_scale            = glm::vec2(1.0f, 1.0f);
    float              transform_rotation_degrees = 0.0f;
    glm::vec2          view_pivot_offset          = glm::vec2(0.0f, 0.0f);
    bool               movementBounce             = false;
    std::optional<int> renderOrder;

    std::string damageSFXPath;
    std::string stepSFXPath;
    std::string dialogueSFXPath;

    bool hasSpoken = false;

    bool lastLooked = false; // up is true, down is false

    std::unordered_map<std::string, std::shared_ptr<luabridge::LuaRef>> components;
};
