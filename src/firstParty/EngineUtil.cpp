#include "EngineUtil.hpp"
#include "Engine.hpp"
#include "ImageDB.hpp"
#include "Renderer.hpp"
#include "SceneLoader.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

void EngineUtil::startup() {

    // read the game.config file

    rapidjson::Document document;

    // Check if resources folder exists
    if (!std::filesystem::exists("resources")) {
        std::cout << "error: resources/ missing";
        exit(0);
    }

    // Check for a game.config file in resources folder
    if (!std::filesystem::exists("resources/game.config")) {
        std::cout << "error: resources/game.config missing";
        exit(0);
    }

    EngineUtil::ReadJsonFile("resources/game.config", document);

    // WINDOW SETUP
    if (document.HasMember("game_title")) {
        std::string game_title = document["game_title"].GetString();
        // change to char* dont use strcpy
        Engine::setGameTitle(game_title);
    }

    if (document.HasMember("player_movement_speed")) {
        float player_movement_speed = document["player_movement_speed"].GetFloat();
        Engine::setPlayerSpeed(player_movement_speed);
    }

    // if resources/rendering.config exists, and x_resolution and y_resolution are set, set the camera size to those values
    if (std::filesystem::exists("resources/rendering.config")) {
        rapidjson::Document rendering_document;
        EngineUtil::ReadJsonFile("resources/rendering.config", rendering_document);

        if (rendering_document.HasMember("x_resolution")) {
            int x_resolution = rendering_document["x_resolution"].GetInt();
            Engine::setXResolution(x_resolution);
        }

        if (rendering_document.HasMember("y_resolution")) {
            int y_resolution = rendering_document["y_resolution"].GetInt();
            Engine::setYResolution(y_resolution);
        }

        int r = 255;
        int g = 255;
        int b = 255;

        if (rendering_document.HasMember("clear_color_r")) {
            r = rendering_document["clear_color_r"].GetInt();
        }

        if (rendering_document.HasMember("clear_color_g")) {
            g = rendering_document["clear_color_g"].GetInt();
        }

        if (rendering_document.HasMember("clear_color_b")) {
            b = rendering_document["clear_color_b"].GetInt();
        }

        Engine::setClearedColor(r, g, b);

        if (rendering_document.HasMember("zoom_factor")) {
            float zoomFactor = rendering_document["zoom_factor"].GetFloat();
            Engine::setZoomFactor(zoomFactor);
        }

        if (rendering_document.HasMember("cam_ease_factor")) {
            float camEaseFactor = rendering_document["cam_ease_factor"].GetFloat();
            Engine::setCamEaseFactor(camEaseFactor);
        }
    }

    if (!document.HasMember("initial_scene")) {
        std::cout << "error: initial_scene unspecified";
        exit(0);
    }

    std::string initial_scene = document["initial_scene"].GetString();

    std::string scene_path = "resources/scenes/" + initial_scene + ".scene";

    if (!std::filesystem::exists(scene_path)) {
        std::cout << "error: scene " << initial_scene << " is missing";
        exit(0);
    }

    Renderer::init();

    Engine::setCenterX(Engine::getXResolution() / 2);
    Engine::setCenterY(Engine::getYResolution() / 2);

    SceneLoader::loadActors(scene_path);

    for (auto &actor : SceneLoader::loadedActors) {
        Engine::addActor(actor);
    }

    ImageDB::loadHUD();
}
//
