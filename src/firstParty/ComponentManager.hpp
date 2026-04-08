#pragma once

#include "lua.hpp"
#include <algorithm>
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

    static bool IsComponentLoaded(const std::string &componentName);

    static std::shared_ptr<luabridge::LuaRef> GetComponent(const std::string &componentName);

    static void addComponentToCache(const std::string &componentName, std::shared_ptr<luabridge::LuaRef> component);

    static void setLuaState(lua_State *luaState);

    static lua_State *getLuaState();

    static void ReportError(const std::string &actor_name, const luabridge::LuaException &e);

    static void sortQueues();

    static void Quit();
    static void Sleep(int milliseconds);
    static int  GetFrame();
    static void OpenURL(const std::string &url);

    static luabridge::LuaRef GetComponentType(std::string typeName);

    static void scheduleRuntimeComponent(Actor            *a,
                                         std::string       key,
                                         luabridge::LuaRef inst);
    static void flushPending(); // call at start of next frame

    static void ResetLifecycleQueues();

    static void scheduleComponentRemoval(Actor *actor, std::string key);

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

    static inline std::vector<std::pair<Actor *, std::string>> pendingRemovals;

    static void flushPendingRemovals();

    inline static lua_State *L;
};
