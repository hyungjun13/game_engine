//
//  Engine.hpp
//  game_engine
//
//  Created  by Hyungjun Kim on 25/1/2025.
//

#pragma once

#ifndef Engine_hpp
#define Engine_hpp

#endif /* Engine_hpp */

#include "lua.hpp"

#include "LuaBridge.h"

#include "Actor.hpp"

#include "glm/glm.hpp"

#include "SDL2/SDL.h"

#include <deque>
#include <unordered_map>
#include <vector>

struct imageDrawRequest {
    SDL_Texture *texture;
    SDL_FRect   *srcrect;
    SDL_FRect    dstrect;
    Actor       *actor = nullptr;
};

struct textRequest {
    std::string text;
    int         fontSize;
    SDL_Color   color;
    int         x;
    int         y;
};

class Engine {

  public:
    static void GameLoop();

    static void Input();
    static void Update();
    static void LateUpdate();
    static void Render();

    //    static bool isBlocked(Actor &player, glm::vec2 &delta);

    // static bool checkNPCmovement(Actor &actor);

    static void addActor(Actor &actor);

    static void        setGameTitle(std::string title);
    static std::string getGameTitle();

    static void setXResolution(int n);
    static void setYResolution(int n);
    static int  getXResolution();
    static int  getYResolution();

    static void setClearedColor(int r, int g, int b);
    static int  getClearedColorR();
    static int  getClearedColorG();
    static int  getClearedColorB();

    static void RenderIntro();

    static int  getGameState();
    static void setGameState(int state);

    static void renderImages();
    static void renderActor(Actor &actor);
    static void renderTexts();

    static void addTextureToCache(std::string name, SDL_Texture *texture);

    static void      setCameraPosition(glm::vec2 position);
    static glm::vec2 getCameraPosition();

    static void setCenterX(int x);
    static void setCenterY(int y);
    static int  getCenterX();
    static int  getCenterY();

    static void setHasPlayer(bool b);
    static bool getHasPlayer();

    static void renderHUD();
    static void renderDialogue();

    static void renderOutro(int index);

    static void  setZoomFactor(float scale);
    static float getZoomFactor();

    static void renderHUDQueue();

    static void  setPlayerSpeed(float speed);
    static float getPlayerSpeed();

    static SDL_Texture *getTextTexture(const std::string &text, int fontSize, const SDL_Color &color);
    static void         clearTextTextureCache(); // Call this if you need to update text frequently

    static void  setCamEaseFactor(float cam_ease_factor);
    static float getCamEaseFactor();

    static void updateActors();

    static std::unordered_map<std::string, SDL_Texture *> *getTextureCache() {
        return &textureCache;
    }

    static luabridge::LuaRef Find(const std::string &name) {
        for (auto &actor : masterActorList) {
            if (actor.getName() == name) {
                // need to return the refreence to the  actor
                return luabridge::LuaRef(ComponentManager::getLuaState(), &actor);
            }
        }
        return luabridge::LuaRef(ComponentManager::getLuaState());
    }

    static luabridge::LuaRef FindAll(const std::string &name) {
        luabridge::LuaRef resultTable = luabridge::newTable(ComponentManager::getLuaState());
        int               index       = 1;

        for (auto &actor : masterActorList) {
            if (actor.getName() == name) {
                resultTable[index] = luabridge::LuaRef(ComponentManager::getLuaState(), &actor);
                index++;
            }
        }
        return resultTable;
    }

  private:
    inline static const int UNIT = 100;

    inline static bool hasPlayer = false;

    inline static bool running = true;

    inline static bool hasOutroPlayed = false;

    inline static glm::vec2 delta = glm::vec2(0.0f, 0.0f);

    inline static std::string gameTitle   = "";
    inline static int         xResolution = 640;
    inline static int         yResolution = 360;

    inline static int clear_color_r = 255;
    inline static int clear_color_g = 255;
    inline static int clear_color_b = 255;

    inline static int gameState = 0; // 0 = intro, 1 = game, 2 = game over

    inline static glm::vec2 playerPosition = glm::vec2(0.0f, 0.0f);
    inline static int       playerHealth   = 3;
    inline static int       playerScore    = 0;

    inline static glm::vec2 cameraPosition = glm::vec2(0.0f, 0.0f);
    inline static int       centerX        = 0;
    inline static int       centerY        = 0;

    inline static bool winFlag      = false;
    inline static bool deathFlag    = false;
    inline static bool endFlag      = false;
    inline static bool newSceneFlag = false;

    inline static std::string newScene = "";

    inline static bool findActorInVector(std::vector<Actor *> actors, Actor *actor);

    inline static std::vector<Actor> masterActorList;

    inline static int introImgIndex  = 0;
    inline static int introTextIndex = 0;

    inline static std::deque<imageDrawRequest> imageDrawQueue;
    inline static std::deque<imageDrawRequest> imageDrawQueueHUD;
    inline static std::vector<textRequest>     textDrawQueue;
    inline static std::vector<textRequest>     textDrawQueueHUD;

    inline static std::unordered_map<std::string, SDL_Texture *> textureCache;

    inline static std::unordered_map<std::string, SDL_Texture *> textTextureCache;

    // Helper function to generate a unique key for text.
    inline static std::string generateTextKey(const std::string &text, int fontSize, const SDL_Color &color) {
        return text + "_" + std::to_string(fontSize) + "_" +
               std::to_string(color.r) + "_" +
               std::to_string(color.g) + "_" +
               std::to_string(color.b) + "_" +
               std::to_string(color.a);
    }

    inline static int currentFrame = 0;

    inline static float zoomFactor = 1.0f;

    inline static float camEaseFactor = 1.0f;

    inline static float playerSpeed = 0.02f;
};
