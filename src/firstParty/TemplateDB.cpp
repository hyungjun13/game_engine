#include "TemplateDB.hpp"
#include "ComponentManager.hpp"
#include "EngineUtil.hpp"
#include "Helper.h"
#include "Renderer.hpp"
#include "SDL2/SDL.h"
#include "lua.hpp"
#include "rapidjson/document.h"

#include <filesystem>
#include <iostream>
#include <string>

#include "LuaBridge.h"

void TemplateDB::loadTemplate(std::string templateName, Actor &actor) {
    std::string templatePath = "resources/actor_templates/" + templateName + ".template";

    if (!std::filesystem::exists(templatePath)) {
        std::cout << "error: template " << templateName << " is missing" << std::endl;
        exit(0);
    }

    rapidjson::Document document;
    EngineUtil::ReadJsonFile(templatePath, document);

    if (document.HasMember("name")) {
        actor.setName(document["name"].GetString());
    }

    // Process components if any.
    if (document.HasMember("components") && document["components"].IsObject()) {
        const rapidjson::Value &components = document["components"];
        // For templates we do not queue OnStart, because we want the OnStart to be called
        // only on the actor-level instance (if overridden).
        for (rapidjson::Value::ConstMemberIterator itr = components.MemberBegin();
             itr != components.MemberEnd(); ++itr) {
            std::string             compKey = itr->name.GetString();
            const rapidjson::Value &compObj = itr->value;

            if (!compObj.HasMember("type")) {
                std::cout << "error: component " << compKey << " missing type";
                exit(0);
            }
            std::string compType = compObj["type"].GetString();

            std::string luaComponentPath = "resources/component_types/" + compType + ".lua";
            if (!std::filesystem::exists(luaComponentPath)) {
                std::cout << "error: failed to locate component " << compType;
                exit(0);
            }

            lua_State *L          = ComponentManager::getLuaState();
            int        loadStatus = luaL_dofile(L, luaComponentPath.c_str());
            if (loadStatus != LUA_OK) {
                std::cout << "problem with lua file " << compType;
                exit(0);
            }

            luabridge::LuaRef baseComponent = luabridge::getGlobal(L, compType.c_str());
            if (!baseComponent.isTable()) {
                std::cout << "error: component " << compType << " is not a valid table";
                exit(0);
            }

            luabridge::LuaRef instance = luabridge::newTable(L);
            ComponentManager::EstablishInheritance(instance, baseComponent);
            instance["key"]     = compKey;
            instance["type"]    = compType;
            instance["enabled"] = true;

            // Process any default properties from the template.
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
            actor.injectConveninenceReferences(compPtr);
            actor.addComponent(compKey, compPtr);
            // Note: We do NOT queue OnStart for template com ponents.
        }
    }
}
