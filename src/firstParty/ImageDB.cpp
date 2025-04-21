#include "ImageDB.hpp"
#include "EngineUtil.hpp"
#include "Helper.h"
#include "Renderer.hpp"

#include "SDL2/SDL.h"
#include "SDL_image/SDL_image.h"
#include "rapidjson/document.h"

#include <filesystem>
#include <unordered_map>

void ImageDB::check() {
    /*When the game begins, the user may want to display some intro screen images.
These images, if they exist, are configured in game.config as intro_image
intro_image is an array of strings– not a singular string.
Image names (with .png extension) must exist within resources/images/
If the image doesn’t exist, print error: missing image <image_name>
(then exit the program immediately with code 0)
*/

    // read the game.config file

    rapidjson::Document document;

    EngineUtil::ReadJsonFile("resources/game.config", document);

    // Check if intro_image exists
    if (document.HasMember("intro_image")) {
        for (auto &image : document["intro_image"].GetArray()) {
            std::string imageName = image.GetString();
            if (!std::filesystem::exists("resources/images/" + imageName + ".png")) {
                std::cout << "error: missing image " << imageName;
                exit(0);
            }
        }
    }
}

void ImageDB::loadIntroImages() {
    // If intro_image array exists, load each image into memory
    // If intro_image array does not exist, do nothing

    rapidjson::Document document;
    EngineUtil::ReadJsonFile("resources/game.config", document);

    if (document.HasMember("intro_image")) {
        for (auto &image : document["intro_image"].GetArray()) {
            std::string imageName = image.GetString();
            // Load image into memory
            std::string imagePath = "resources/images/" + imageName + ".png";

            SDL_Texture *texture = IMG_LoadTexture(Renderer::getRenderer(), imagePath.c_str());
            // Store texture in map
            introImageCache.push_back(texture);
        }
    }
}

void ImageDB::loadOutroImages() {
    // Only need two, one for game over and one for win
    // Load image into memory
    // Store texture in map

    rapidjson::Document document;
    EngineUtil::ReadJsonFile("resources/game.config", document);
}

void ImageDB::loadHUD() {
    /*This suite contains basic tests exercising your engine’s ability to render gameplay HUD information when a player actor is present in the scene.

hp_image may be configured in game.config representing the image to be used as health icons.
A value of heart is a reference to resources/images/heart.png
If an actor with the name player exists but no hp_image is specified…
Print error: player actor requires an hp_image be defined
(then exit immediately with code 0)
*/
    rapidjson::Document document;
    EngineUtil::ReadJsonFile("resources/game.config", document);

    if (Engine::getHasPlayer()) {
    }
}