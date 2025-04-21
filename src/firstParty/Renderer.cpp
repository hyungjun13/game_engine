#include <filesystem>

#include "Engine.hpp"
#include "EngineUtil.hpp"
#include "Helper.h"
#include "Renderer.hpp"
#include "rapidjson/document.h"

void Renderer::init() {

    window   = Helper::SDL_CreateWindow(Engine::getGameTitle().c_str(), 0, 0, Engine::getXResolution(), Engine::getYResolution(), SDL_WINDOW_SHOWN);
    renderer = Helper::SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
}
