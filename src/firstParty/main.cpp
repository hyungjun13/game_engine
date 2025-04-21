//
//  main.cpp
//  game_engine_hyungjun
//
//  Created by Hyungjun Kim on 25/1/2025.
//

#include <iostream>

#include "lua.hpp"

#include "LuaBridge.h"

#include "AudioDB.hpp"
#include "ComponentManager.hpp"
#include "Engine.hpp"
#include "EngineUtil.hpp"
#include "Helper.h"
#include "ImageDB.hpp"
#include "SDL2/SDL.h"
#include "TextDB.hpp"

void initialize() {
    // Call any initialization functions here, like Renderer::init(), SDL_init, EngineUtil::startup(), etc.

    ComponentManager::Initialize();
    EngineUtil::startup();

    ImageDB::check();
    ImageDB::loadIntroImages();
    ImageDB::loadOutroImages();

    TextDB::check();
    TextDB::loadIntroText();

    AudioDB::init();

}

int main(int argc, char *argv[]) {

    // startup()

    initialize();

    

    Engine::GameLoop();

    // abcd

    return 0;
}
