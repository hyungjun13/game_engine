#include "SceneLoader.hpp"
#include "AudioDB.hpp"
#include "ComponentManager.hpp"
#include "EngineUtil.hpp"
#include "Helper.h"
#include "Renderer.hpp"
#include "SDL2/SDL.h"
#include "TemplateDB.hpp"
#include "lua.hpp"
#include "rapidjson/document.h"

#include "LuaBridge.h"
#include <filesystem>
#include <iostream>
#include <string>

void SceneLoader::loadActors(std::string scene_path) {
    loadedActors.clear();

    rapidjson::Document document;
    EngineUtil::ReadJsonFile(scene_path, document);

    for (auto &actorValue : document["actors"].GetArray()) {
        // Create a new Actor on the heap.
        auto newActor = std::make_shared<Actor>();
        newActor->setId(Engine::idCounter++);

        // Process "template" if present.
        if (actorValue.HasMember("template")) {

            std::string templateName = actorValue["template"].GetString();
            // Pass the Actor by reference (or update loadTemplate to accept shared_ptr)
            TemplateDB::loadTemplate(templateName, *newActor);
        }

        // Process "name".
        if (actorValue.HasMember("name")) {
            newActor->setName(actorValue["name"].GetString());
        }

        // Process components (if any) for this actor.
        if (actorValue.HasMember("components") && actorValue["components"].IsObject()) {
            const rapidjson::Value &components = actorValue["components"];
            // Iterate over each component defined in the actor's "components" object.
            for (rapidjson::Value::ConstMemberIterator itr = components.MemberBegin();
                 itr != components.MemberEnd(); ++itr) {
                std::string             compKey = itr->name.GetString();
                const rapidjson::Value &compObj = itr->value;

                // Determine if this component override provides a "type".
                bool hasType = compObj.HasMember("type");

                // If there's no type and the actor doesn't already have this component, that's an error.
                if (!hasType && newActor->getComponent(compKey) == nullptr) {
                    std::cout << "error: component " << compKey << " missing type" << std::endl;
                    exit(0);
                }

                // Get the Lua state.
                lua_State *L = ComponentManager::getLuaState();

                if (hasType) {
                    std::string compType = compObj["type"].GetString();
                    // Build the Lua file path for the component type.
                    std::string luaComponentPath = "resources/component_types/" + compType + ".lua";
                    if (!std::filesystem::exists(luaComponentPath)) {
                        std::cout << "error: failed to locate component " << compType;
                        exit(0);
                    }

                    // Use the ComponentManager cache: if the base component for compType isn’t loaded yet, load it and cache it.
                    std::shared_ptr<luabridge::LuaRef> baseComponent;
                    if (!ComponentManager::IsComponentLoaded(compType)) {
                        int loadStatus = luaL_dofile(L, luaComponentPath.c_str());
                        if (loadStatus != LUA_OK) {
                            std::cout << "problem with lua file " << compType;
                            exit(0);
                        }
                        baseComponent = std::make_shared<luabridge::LuaRef>(luabridge::getGlobal(L, compType.c_str()));
                        if (!baseComponent->isTable()) {
                            std::cout << "error: component " << compType << " is not a valid table";
                            exit(0);
                        }
                        ComponentManager::addComponentToCache(compType, baseComponent);
                    } else {
                        baseComponent = ComponentManager::GetComponent(compType);
                    }

                    // Create a new component instance.
                    luabridge::LuaRef instance = luabridge::newTable(L);
                    ComponentManager::EstablishInheritance(instance, *baseComponent);
                    instance["key"]        = compKey;
                    instance["type"]       = compType;
                    instance["enabled"]    = true;
                    instance["hasStarted"] = false;

                    // Inject override properties.
                    for (rapidjson::Value::ConstMemberIterator propItr = compObj.MemberBegin();
                         propItr != compObj.MemberEnd(); ++propItr) {
                        std::string propName = propItr->name.GetString();
                        if (propName == "type")
                            continue;
                        if (propItr->value.IsString()) {
                            instance[propName] = propItr->value.GetString();
                        } else if (propItr->value.IsInt()) {
                            instance[propName] = propItr->value.GetInt();
                        } else if (propItr->value.IsFloat()) {
                            instance[propName] = propItr->value.GetFloat();
                        } else if (propItr->value.IsBool()) {
                            instance[propName] = propItr->value.GetBool();
                        }
                    }

                    auto compPtr = std::make_shared<luabridge::LuaRef>(instance);
                    // Inject a reference to the actor into the component.
                    newActor->injectConveninenceReferences(compPtr);
                    newActor->addComponent(compKey, compPtr);
                } else {
                    // Otherwise, this is an override for an existing (inhe rited) component.
                    auto existingComp = newActor->getComponent(compKey);
                    // Update its override properties.
                    for (rapidjson::Value::ConstMemberIterator propItr = compObj.MemberBegin();
                         propItr != compObj.MemberEnd(); ++propItr) {
                        std::string propName = propItr->name.GetString();
                        if (propName == "type")
                            continue;
                        if (propItr->value.IsString()) {
                            (*existingComp)[propName] = propItr->value.GetString();
                        } else if (propItr->value.IsInt()) {
                            (*existingComp)[propName] = propItr->value.GetInt();
                        } else if (propItr->value.IsFloat()) {
                            (*existingComp)[propName] = propItr->value.GetFloat();
                        } else if (propItr->value.IsBool()) {
                            (*existingComp)[propName] = propItr->value.GetBool();
                        }
                    }
                    // No need to queue OnStart here; the existing component should have been queued when it was created.
                }
            }
        }
        // For any components attached to this actor (both inherited and new), queue their lifecycle functions.
        for (const auto &pair : newActor->getComponentsMap()) {
            // Check if the component's "OnStart" field is a function.
            if (((*pair.second)["OnStart"]).isFunction()) {
                ComponentManager::QueueOnStart(newActor->getId(), pair.first, *(pair.second));
            }
            // Check for the "OnUpdate" function.
            if (((*pair.second)["OnUpdate"]).isFunction()) {
                ComponentManager::QueueOnUpdate(newActor->getId(), pair.second);
            }
            // Check for the "OnLateUpdate" function.
            if (((*pair.second)["OnLateUpdate"]).isFunction()) {
                ComponentManager::QueueOnLateUpdate(newActor->getId(), pair.second);
            }
        }

        loadedActors.push_back(newActor);

        if (newActor->getName() == "player") {
            Engine::setHasPlayer(true);
        }
    }

    // after loading all actors, sort the components in the queues in componentmanager by their key
    ComponentManager::sortQueues();
}
