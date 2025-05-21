#include "ComponentManager.hpp"
#include "Engine.hpp"
#include "Input.hpp"

#include <filesystem>
#include <iostream>
#include <thread>

#include "Helper.h"

// Initialization functions remain unchanged.
void ComponentManager::Initialize() {
    InitializeState();
    InitializeFunctions();
    InitializeComponents();
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

    // Example registration for glm::vec2 and  Actor remains the same.
    luabridge::getGlobalNamespace(L)
        .beginClass<glm::vec2>("vec2")
        .addData("x", &glm::vec2::x)
        .addData("y", &glm::vec2::y)
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
        .endClass();

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Actor")
        .addFunction("Find", &Engine::Find)
        .addFunction("FindAll", &Engine::FindAll)
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
}

void ComponentManager::InitializeComponents() {
    lua_State  *L            = getLuaState();
    std::string componentDir = "resources/component_types";

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
    // Resize the vector if necessary.
    if (actorIndex >= onStartQueue.size()) {
        onStartQueue.resize(actorIndex + 1);
    }
    ComponentOnStartEntry entry = {key, instance};
    onStartQueue[actorIndex].push(entry);
}

void ComponentManager::ProcessOnStart() {
    // Process each actor's PQ separately.
    for (auto &pq : onStartQueue) {
        while (!pq.empty()) {
            ComponentOnStartEntry entry = pq.top();
            pq.pop();
            // check if enabled
            if (entry.componentFunction["OnStart"].isFunction() && entry.componentFunction["enabled"].cast<bool>()) {
                try {

                    entry.componentFunction["OnStart"](entry.componentFunction);
                } catch (const luabridge::LuaException &e) {
                    // Report the error.
                    luabridge::LuaRef actorRef = entry.componentFunction["actor"];
                    Actor            *actorPtr = actorRef.cast<Actor *>();
                    ReportError(actorPtr->getName(), e);
                }
            }
        }
    }
    // Clear the vector after processing.
    onStartQueue.clear();
}

void ComponentManager::QueueOnUpdate(int actorIndex, std::shared_ptr<luabridge::LuaRef> instance) {
    // Resize the vector if necessary.
    if (actorIndex >= onUpdateQueue.size()) {
        onUpdateQueue.resize(actorIndex + 1);
    }
    onUpdateQueue[actorIndex].push_back(instance);
}

void ComponentManager::QueueOnLateUpdate(int actorIndex, std::shared_ptr<luabridge::LuaRef> instance) {
    // Resize the vector if necessary.
    if (actorIndex >= onLateUpdateQueue.size()) {
        onLateUpdateQueue.resize(actorIndex + 1);
    }
    onLateUpdateQueue[actorIndex].push_back(instance);
}
void ComponentManager::ProcessOnUpdate() {
    for (auto &actorQueue : onUpdateQueue) {
        for (auto &instance : actorQueue) {
            // Check that the OnUpdate field is a function.
            if (((*instance)["OnUpdate"]).isFunction() && (*instance)["enabled"].cast<bool>()) {

                try {
                    // Call onUpdate and pass in ac tor instance as an argument.
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
            // Check that the OnLateUpdate field is a function.
            if (((*instance)["OnLateUpdate"]).isFunction() && (*instance)["enabled"].cast<bool>()) {

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

    if (defaultComponents.find(typeName) != defaultComponents.end()) {
        return *(defaultComponents[typeName]);
    } else {
        return luabridge::LuaRef(getLuaState());
    }
}

void ComponentManager::scheduleRuntimeComponent(Actor            *a,
                                                std::string       key,
                                                luabridge::LuaRef inst) {
    pendingAdds.push_back({a, std::move(key), std::move(inst)});
}

void ComponentManager::flushPending() {
    for (auto &p : pendingAdds) {
        // actually insert into the actor’s map
        p.actor->getComponentsMap()[p.key] = std::make_shared<luabridge::LuaRef>(p.instance);
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
