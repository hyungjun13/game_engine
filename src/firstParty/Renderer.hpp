#pragma once

#include "Engine.hpp"
#include "SDL2/SDL.h"
#include "glm/glm.hpp"

#include <string>

class Renderer {
  public:
    static void init();

    static void setGameTitle(std::string title) {
        game_title = title;
    }

    static SDL_Renderer *getRenderer() {
        return renderer;
    }

    static void setRenderer(SDL_Renderer *r) {
        renderer = r;
    }

    static SDL_Window *getWindow() {
        return window;
    }

    static void setWindow(SDL_Window *w) {
        window = w;
    }

  private:
    inline static SDL_Window   *window   = nullptr;
    inline static SDL_Renderer *renderer = nullptr;

    inline static glm::vec2 windowPosition = glm::vec2(0, 0);
    inline static glm::vec2 windowSize     = glm::vec2(640, 360);
    inline static float     zoomFactor     = 1.0f;

    inline static glm::vec2 cameraPosition = glm::vec2(0, 0);

    inline static uint8_t clearColorR = 255;
    inline static uint8_t clearColorG = 255;
    inline static uint8_t clearColorB = 255;
    inline static uint8_t clearColorA = 255;

    inline static bool drawDebug = false;

    static std::string game_title;
};