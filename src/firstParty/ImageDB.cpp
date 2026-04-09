#include "ImageDB.hpp"
#include "Engine.hpp"
#include "EngineUtil.hpp"
#include "Helper.h"
#include "Renderer.hpp"

#include "SDL2/SDL.h"
#include "SDL_image/SDL_image.h"
#include "rapidjson/document.h"

#include <algorithm>
#include <filesystem>
#include <unordered_map>

namespace {
constexpr int PIXELS_PER_METER = 100;

int clampByte(int value) {
    if (value < 0)
        return 0;
    if (value > 255)
        return 255;
    return value;
}

bool compare_image_requests(const ImageDrawRequest &a, const ImageDrawRequest &b) {
    return a.sorting_order < b.sorting_order;
}
} // namespace

std::vector<SDL_Texture *> &ImageDB::getIntroImageCache() {
    return introImageCache;
}

std::array<SDL_Texture *, 2> &ImageDB::getOutroImageCache() {
    return outroImageCache;
}

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

SDL_Texture *ImageDB::getImage(std::string imageName) {
    auto it = imageCache.find(imageName);
    if (it != imageCache.end()) {
        return it->second;
    }

    std::string imagePath = "resources/images/" + imageName + ".png";
    if (!std::filesystem::exists(imagePath)) {
        std::cout << "error: missing image " << imageName;
        exit(0);
    }

    SDL_Texture *texture = IMG_LoadTexture(Renderer::getRenderer(), imagePath.c_str());
    if (!texture) {
        std::cout << "error: missing image " << imageName;
        exit(0);
    }

    imageCache[imageName] = texture;
    return texture;
}

void ImageDB::Draw(const std::string &imageName,
                   float              x,
                   float              y,
                   float              rotationDegrees,
                   float              scaleX,
                   float              scaleY,
                   float              pivotX,
                   float              pivotY,
                   float              r,
                   float              g,
                   float              b,
                   float              a,
                   float              sortingOrder) {
    ImageDrawRequest request;
    request.image_name       = imageName;
    request.x                = x;
    request.y                = y;
    request.rotation_degrees = static_cast<int>(rotationDegrees);
    request.scale_x          = scaleX;
    request.scale_y          = scaleY;
    request.pivot_x          = pivotX;
    request.pivot_y          = pivotY;
    request.r                = clampByte(static_cast<int>(r));
    request.g                = clampByte(static_cast<int>(g));
    request.b                = clampByte(static_cast<int>(b));
    request.a                = clampByte(static_cast<int>(a));
    request.sorting_order    = static_cast<int>(sortingOrder);

    imageDrawQueue.push_back(request);
}

void ImageDB::DrawUI(const std::string &imageName,
                     float              x,
                     float              y) {
    DrawUIEx(imageName, x, y, 255.0f, 255.0f, 255.0f, 255.0f, 0.0f);
}

void ImageDB::DrawUIEx(const std::string &imageName,
                       float              x,
                       float              y,
                       float              r,
                       float              g,
                       float              b,
                       float              a,
                       float              sortingOrder) {
    SDL_Texture *texture = getImage(imageName);

    Engine::QueueHUDImageDraw(texture,
                              x,
                              y,
                              clampByte(static_cast<int>(r)),
                              clampByte(static_cast<int>(g)),
                              clampByte(static_cast<int>(b)),
                              clampByte(static_cast<int>(a)),
                              static_cast<int>(sortingOrder));
}

void ImageDB::DrawPixel(float x,
                        float y,
                        float r,
                        float g,
                        float b,
                        float a) {
    PixelDrawRequest request;
    request.x = static_cast<int>(x);
    request.y = static_cast<int>(y);
    request.r = clampByte(static_cast<int>(r));
    request.g = clampByte(static_cast<int>(g));
    request.b = clampByte(static_cast<int>(b));
    request.a = clampByte(static_cast<int>(a));
    pixelDrawQueue.push_back(request);
}

void ImageDB::RenderAndClearAllImages() {
    std::stable_sort(imageDrawQueue.begin(), imageDrawQueue.end(), compare_image_requests);

    float zoomFactor = Engine::getZoomFactor();
    SDL_RenderSetScale(Renderer::getRenderer(), zoomFactor, zoomFactor);

    for (auto &request : imageDrawQueue) {
        SDL_Texture *tex = getImage(request.image_name);

        SDL_Rect texRect;
        SDL_QueryTexture(tex, nullptr, nullptr, &texRect.w, &texRect.h);

        int flipMode = SDL_FLIP_NONE;
        if (request.scale_x < 0.0f) {
            flipMode |= SDL_FLIP_HORIZONTAL;
        }
        if (request.scale_y < 0.0f) {
            flipMode |= SDL_FLIP_VERTICAL;
        }

        float xScale = std::abs(request.scale_x);
        float yScale = std::abs(request.scale_y);

        texRect.w = static_cast<int>(texRect.w * xScale);
        texRect.h = static_cast<int>(texRect.h * yScale);

        SDL_Point pivotPoint = {
            static_cast<int>(request.pivot_x * texRect.w),
            static_cast<int>(request.pivot_y * texRect.h)};

        glm::vec2 finalRenderingPosition = glm::vec2(request.x, request.y) - Engine::getCameraPosition();

        texRect.x = static_cast<int>(finalRenderingPosition.x * PIXELS_PER_METER + Engine::getXResolution() * 0.5f * (1.0f / zoomFactor) - pivotPoint.x);
        texRect.y = static_cast<int>(finalRenderingPosition.y * PIXELS_PER_METER + Engine::getYResolution() * 0.5f * (1.0f / zoomFactor) - pivotPoint.y);

        SDL_SetTextureColorMod(tex, static_cast<Uint8>(request.r), static_cast<Uint8>(request.g), static_cast<Uint8>(request.b));
        SDL_SetTextureAlphaMod(tex, static_cast<Uint8>(request.a));

        SDL_RenderCopyEx(Renderer::getRenderer(), tex, nullptr, &texRect, request.rotation_degrees, &pivotPoint, static_cast<SDL_RendererFlip>(flipMode));

        SDL_SetTextureColorMod(tex, 255, 255, 255);
        SDL_SetTextureAlphaMod(tex, 255);
    }

    SDL_RenderSetScale(Renderer::getRenderer(), 1.0f, 1.0f);
    imageDrawQueue.clear();
}

void ImageDB::RenderAndClearAllPixels() {
    SDL_Renderer *renderer = Renderer::getRenderer();
    for (const auto &request : pixelDrawQueue) {
        SDL_SetRenderDrawColor(renderer,
                               static_cast<Uint8>(request.r),
                               static_cast<Uint8>(request.g),
                               static_cast<Uint8>(request.b),
                               static_cast<Uint8>(request.a));
        SDL_RenderDrawPoint(renderer, request.x, request.y);
    }

    pixelDrawQueue.clear();
}