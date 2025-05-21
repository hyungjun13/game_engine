#pragma once

#include "lua.hpp"
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "Actor.hpp"
#include "LuaBridge.h"

// #include "SceneLoader.hpp"

struct ComponentOnStartEntry {
    std::string       key;
    luabridge::LuaRef componentFunction;
};

struct ComponentComparator {
    // This creates a min‐heap (smallest key comes first).
    bool operator()(const ComponentOnStartEntry &a, const ComponentOnStartEntry &b) const {
        return a.key > b.key;
    }
};

struct PendingComponent {
    Actor            *actor;
    std::string       key;
    luabridge::LuaRef instance;
};

class ComponentManager {
  public:
    static void Initialize();
    static void EstablishInheritance(luabridge::LuaRef &instance_table, luabridge::LuaRef &parent_table);

    // Now include an actorIndex so that each actor’s components go into its own PQ.
    static void QueueOnStart(int actorIndex, const std::string &key, luabridge::LuaRef instance);
    static void ProcessOnStart();

    static void QueueOnUpdate(int actorIndex, std::shared_ptr<luabridge::LuaRef> instance);
    static void ProcessOnUpdate();

    static void QueueOnLateUpdate(int actorIndex, std::shared_ptr<luabridge::LuaRef> instance);
    static void ProcessOnLateUpdate();

    static int DebugLog(lua_State *L);

    static bool IsComponentLoaded(const std::string &componentName) {
        return loadedComponentCache.find(componentName) != loadedComponentCache.end();
    }

    static std::shared_ptr<luabridge::LuaRef> GetComponent(const std::string &componentName) {
        return loadedComponentCache[componentName];
    }

    static void addComponentToCache(const std::string &componentName, std::shared_ptr<luabridge::LuaRef> component) {
        loadedComponentCache[componentName] = component;
    }

    static void setLuaState(lua_State *luaState) {
        L = luaState;
    }

    static lua_State *getLuaState() {
        return L;
    }

    static void ReportError(const std::string &actor_name, const luabridge::LuaException &e) {
        std::string error_message = e.what();

        std::replace(error_message.begin(), error_message.end(), '\\', '/');

        std::cout << "\033[31m" << actor_name << " : " << error_message << "\033[0m" << std::endl;
    }

    static void sortQueues() {
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

    static void Quit();
    static void Sleep(int milliseconds);
    static int  GetFrame();
    static void OpenURL(const std::string &url);

    static luabridge::LuaRef GetComponentType(std::string typeName);

    static void scheduleRuntimeComponent(Actor            *a,
                                         std::string       key,
                                         luabridge::LuaRef inst);
    static void flushPending(); // call at start of next frame

  private:
    static void InitializeState();
    static void InitializeFunctions();
    static void InitializeComponents();

    // For each actor (by its loading order index), we store a PQ of its components.
    static inline std::vector<std::priority_queue<ComponentOnStartEntry, std::vector<ComponentOnStartEntry>, ComponentComparator>> onStartQueue;
    static inline std::unordered_map<std::string, std::shared_ptr<luabridge::LuaRef>>                                              loadedComponentCache;
    static inline std::unordered_map<std::string, std::shared_ptr<luabridge::LuaRef>>                                              defaultComponents; // Loaded in InitializeComponents

    static inline std::vector<std::vector<std::shared_ptr<luabridge::LuaRef>>> onUpdateQueue; // queue[actor][component]
    static inline std::vector<std::vector<std::shared_ptr<luabridge::LuaRef>>> onLateUpdateQueue;

    static inline std::vector<PendingComponent> pendingAdds;

    inline static lua_State *L;
};
