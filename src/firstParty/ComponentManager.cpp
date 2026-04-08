#include "ComponentManager.hpp"
#include "AudioDB.hpp"
#include "Engine.hpp"
#include "ImageDB.hpp"
#include "Input.hpp"
#include "TextDB.hpp"

#include <filesystem>
#include <iostream>
#include <thread>

#include "Helper.h"

namespace {
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
        .addFunction("RemoveComponent", &Actor::RemoveComponent)
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
    flushPendingRemovals();
}

void ComponentManager::ResetLifecycleQueues() {
    onStartQueue.clear();
    onUpdateQueue.clear();
    onLateUpdateQueue.clear();
    pendingAdds.clear();
    pendingRemovals.clear();
}

void ComponentManager::scheduleComponentRemoval(Actor *actor, std::string key) {
    pendingRemovals.emplace_back(actor, std::move(key));
}

void ComponentManager::flushPendingRemovals() {
    for (auto &[actor, key] : pendingRemovals) {
        actor->removeComponentByKey(key);
    }
    pendingRemovals.clear();
}