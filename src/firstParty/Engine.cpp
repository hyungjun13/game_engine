
//  Engine.cpp
//  game_engine
//
//  Created by Hyungjun Kim on 25/1/2025.

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <vector>

// #include "Actor.hpp"
#include "AudioDB.hpp"
#include "AudioHelper.h"
#include "ComponentManager.hpp"
#include "Engine.hpp"
#include "Helper.h"
#include "ImageDB.hpp"
#include "Input.hpp"
#include "Renderer.hpp"
#include "SDL2/SDL.h"
#include "SDL2_ttf/SDL_ttf.h"
#include "SceneLoader.hpp"
#include "TextDB.hpp"
#include "glm/glm.hpp"

struct ActorComparator {
    bool operator()(const Actor *a, const Actor *b) const {
        // Use render_order if defined; otherwise, use the actor's y position.
        // Assume that Actor has methods:
        //   int getRenderOrder() const that returns the optional into or the y position.
        //   int getId() const;
        int orderA = a->getRenderOrder();
        int orderB = b->getRenderOrder();

        if (orderA != orderB)
            return orderA < orderB;
        /*Render ordering is still based on y position unless the render_order is specified.
If there are still ties, break them with actor_id
*/
        if (a->getPosition().y != b->getPosition().y)
            return a->getPosition().y < b->getPosition().y;

        return a->getId() < b->getId();
    }
};

uint32_t floatToUint32(float f) {
    union {
        float    f;
        uint32_t i;
    } converter;
    converter.f = f;
    return converter.i;
}

void Engine::GameLoop() {

    Input::Init();

    AudioHelper::Mix_AllocateChannels(50);

    SDL_SetRenderDrawColor(Renderer::getRenderer(), Engine::getClearedColorR(), Engine::getClearedColorG(), Engine::getClearedColorB(), 255);
    SDL_RenderClear(Renderer::getRenderer());

    while (running) {

        Input();

        Update();

        LateUpdate();

        Input::LateUpdate();

        Render();
    }
}

void Engine::Input() {

    SDL_Event e;

    // Process all events using the InputManager.
    while (Helper::SDL_PollEvent(&e)) {
        Input::ProcessEvent(e);
    }
}

void Engine::Update() {

    // Check if quit
    if (Input::GetQuit()) {
        running = false;
    }

    ComponentManager::flushPending();

    currentFrame = Helper::GetFrameNumber();

    if (currentFrame == 0) {
        ComponentManager::ProcessOnStart();
    }

    // Move Actors (only player for now )
    updateActors();
}

void Engine::LateUpdate() {
    ComponentManager::ProcessOnLateUpdate();
}

void Engine::Render() {
    // Clear the renderer.
    SDL_SetRenderDrawColor(Renderer::getRenderer(),
                           Engine::getClearedColorR(),
                           Engine::getClearedColorG(),
                           Engine::getClearedColorB(), 255);
    SDL_RenderClear(Renderer::getRenderer());

    // (Handle new scene loading, etc.)
    if (newSceneFlag) {
        masterActorList.clear();

        std::string scenePath = "resources/scenes/" + newScene + ".scene";
        if (!std::filesystem::exists(scenePath)) {
            std::cout << "error: scene " << newScene << " is missing";
            exit(0);
        }
        SceneLoader::loadActors(scenePath);
        for (auto &actor : SceneLoader::loadedActors) {
            Actor *actorPtr = actor.get();
            Engine::addActor(*actorPtr);
        }
        newSceneFlag = false;
    }

    // --- Render World with Zoom ---
    // Get the zoom factor (make sure zoomFactor is properly initialized to 1.0f by default)
    float zoom = Engine::getZoomFactor(); // e.g., from your configuration
    SDL_RenderSetScale(Renderer::getRenderer(), zoom, zoom);

    // Render world actors using the world queues (imageDrawQueue and textDrawQueue)

    std::multiset<Actor *, ActorComparator> sortedActors;
    for (auto &actor : masterActorList) {
        sortedActors.insert(&actor);
    }
    // Enqueue world actor draw requests.
    for (Actor *actorPtr : sortedActors) {
        renderActor(*actorPtr);
    }
    // Render any queued world images and texts.
    renderImages();
    renderTexts();

    // Reset scale for HUD rendering so that UI elements render at their intended size.
    SDL_RenderSetScale(Renderer::getRenderer(), 1.0f, 1.0f);

    if (getHasPlayer()) {
        renderHUD();      // Uses its own separate HUD queue.
        renderDialogue(); // Also part of the HUD/UI.
    }

    renderImages();
    renderTexts();

    // Finally, render the HUD images from the separate HUD queue.
    renderHUDQueue();

    // debug debug
    // debugDrawColliders();
    // debugDrawViewSize();
    // debugDrawColliderGrid();
    // debugDrawTriggers();
    // debugDrawTriggerGrid();

    Helper::SDL_RenderPresent(Renderer::getRenderer());
}

void Engine::updateActors() {

    // for all things in the onUpdate queue, call the update function
    ComponentManager::ProcessOnUpdate();
}

void Engine::RenderIntro() {
    // Render intro images
    // If intro_image array exists, load each image into memory
    // If intro_image array does not exist, do nothingg

    auto &imageCache = ImageDB::getIntroImageCache();
    auto &textCache  = TextDB::getIntroTextCache();

    if (imageCache.size() > introImgIndex) {
        imageDrawRequest request;

        request.texture = imageCache[introImgIndex];

        SDL_FRect dstrect;
        dstrect.x       = 0;
        dstrect.y       = 0;
        dstrect.w       = Engine::getXResolution();
        dstrect.h       = Engine::getYResolution();
        request.dstrect = dstrect;

        SDL_FRect *srcrect = nullptr;
        request.srcrect    = srcrect;

        imageDrawQueue.push_back(request);

    } else {
        imageDrawRequest request;

        request.texture = imageCache[imageCache.size() - 1];
        SDL_FRect dstrect;
        dstrect.x       = 0;
        dstrect.y       = 0;
        dstrect.w       = Engine::getXResolution();
        dstrect.h       = Engine::getYResolution();
        request.dstrect = dstrect;

        SDL_FRect *srcrect = nullptr;
        request.srcrect    = srcrect;

        imageDrawQueue.push_back(request);
    }

    if (textCache.size() > 0) {

        if (introTextIndex < textCache.size()) {

            textRequest request;
            request.text     = textCache[introTextIndex];
            request.fontSize = 16;
            request.color    = {255, 255, 255, 255};
            request.x        = 25;
            request.y        = Engine::getYResolution() - 50;

            textDrawQueue.push_back(request);
        } else {
            textRequest request;
            request.text     = textCache[textCache.size() - 1];
            request.fontSize = 16;
            request.color    = {255, 255, 255, 255};
            request.x        = 25;
            request.y        = Engine::getYResolution() - 50;
            //
            textDrawQueue.push_back(request);
        }
    }
}

void Engine::renderHUD() {

    // Enqueue one health icon per health point.
    // The starting position is (5,25), and each subsequent icon is offset horizontally by (hpW + 5) pixels.

    // textDrawQueueHUD.push_back(scoreReq);
}

void Engine::renderDialogue() {

    // Build a vector of actors with non-empty dialogue.
    std::vector<Actor *> dialogueActors;

    // find player, and check its trigger set, and then check if any of the actors in the set have dialogue
    for (auto &player : masterActorList) {

        if (player.getName() == "player") {
        }
    }

    int m = dialogueActors.size();
    int i = 0;
    // Now iterate over only those actors with dialogue.
    for (auto &actor : dialogueActors) {
        textRequest request;

        request.text = actor->getNearbyDialogue();

        request.fontSize = 16;
        request.color    = {255, 255, 255, 255};
        request.x        = 25;
        // The bottom-most dialogue message appears at y = (y_resolution - 50),
        // and each previous message is 50 pixels higher.
        request.y = Engine::getYResolution() - 50 - (i * 50);

        textDrawQueue.push_back(request);
        i++; // Increment for each valid dialogue message.
    }
}
void Engine::renderImages() {
    for (auto &request : imageDrawQueue) {
        if (request.actor == nullptr) {
            if (request.srcrect != nullptr) {
                Helper::SDL_RenderCopy(Renderer::getRenderer(), request.texture, request.srcrect, &request.dstrect);
            } else {
                Helper::SDL_RenderCopy(Renderer::getRenderer(), request.texture, NULL, &request.dstrect);
            }
        } else {
            Actor *actor = request.actor;

            // Retrieve scale factors
            float scaleX = actor->getTransformScale().x;
            float scaleY = actor->getTransformScale().y;

            // Determine flip flags while converting scale to absolute values.

            SDL_RendererFlip flip = SDL_FLIP_NONE;
            if (scaleX < 0) {
                flip   = (SDL_RendererFlip)(flip | SDL_FLIP_HORIZONTAL);
                scaleX = -scaleX;
            }
            if (scaleY < 0) {
                flip   = (SDL_RendererFlip)(flip | SDL_FLIP_VERTICAL);
                scaleY = -scaleY;
            }

            // Compute scaled texture size in pixels.
            float scaledWidth  = actor->getViewSize().x * scaleX;
            float scaledHeight = actor->getViewSize().y * scaleY;

            // Compute the scaled pivot offset the same way as debugDrawViewSize:
            // (No additional adjustment for flipping.)
            float scaledPivotX = actor->getViewPivotOffset().x * scaleX;
            float scaledPivotY = actor->getViewPivotOffset().y * scaleY;

            // Compute the destination rectangle so that the pivot point lands at the actor's world position.
            SDL_FRect dstrect;
            glm::vec2 camPos = Engine::getCameraPosition(); // in world units

            float posX = (actor->getPosition().x - camPos.x) * UNIT;
            float posY = (actor->getPosition().y - camPos.y) * UNIT;
            dstrect.x  = (centerX / zoomFactor) + posX - scaledPivotX;
            dstrect.y  = (centerY / zoomFactor) + posY - scaledPivotY;
            dstrect.w  = scaledWidth;
            dstrect.h  = scaledHeight;

            // Use the raw scaled pivot offset as the rotation pivot.
            SDL_FPoint pivot;
            pivot.x = scaledPivotX;
            pivot.y = scaledPivotY;

            if (actor->getMovementBounce() && glm::length(actor->getVelocity()) > 0.0f) {
                // Compute bounce offset in pixels based on the current frame.
                float bounceOffset = -glm::abs(glm::sin(currentFrame * 0.15f)) * 10.0f;
                dstrect.y += bounceOffset; // Only affect rendering, not actor position
            }

            Helper::SDL_RenderCopyEx(actor->getId(), actor->getName(),
                                     Renderer::getRenderer(),
                                     actor->getView(), nullptr, &dstrect,
                                     actor->getTransformRotationDegrees(),
                                     &pivot, flip);
        }
    }
    imageDrawQueue.clear();
}

void Engine::renderTexts() {
    for (auto &request : textDrawQueue) {
        SDL_Surface *surface = TTF_RenderText_Solid(TextDB::getFont(), request.text.c_str(), request.color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(Renderer::getRenderer(), surface);

        SDL_FRect dstrect;
        dstrect.x = request.x;
        dstrect.y = request.y;
        dstrect.w = surface->w;
        dstrect.h = surface->h;

        Helper::SDL_RenderCopy(Renderer::getRenderer(), texture, NULL, &dstrect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
    textDrawQueue.clear();
}

void Engine::renderHUDQueue() {

    for (auto &request : imageDrawQueueHUD) {
        if (request.srcrect != nullptr) {
            Helper::SDL_RenderCopy(Renderer::getRenderer(), request.texture, request.srcrect, &request.dstrect);
        } else {
            Helper::SDL_RenderCopy(Renderer::getRenderer(), request.texture, nullptr, &request.dstrect);
        }
    }
    imageDrawQueueHUD.clear();

    // Render text using cached textures.
    for (auto &request : textDrawQueueHUD) {

        std::string reqText     = request.text;
        int         reqFontSize = request.fontSize;
        SDL_Color   reqColor    = request.color;

        SDL_Texture *texture = getTextTexture(reqText, reqFontSize, reqColor);
        if (!texture)
            continue; // Skip if texture creation failed

        SDL_FRect dstrect;
        dstrect.x = request.x;
        dstrect.y = request.y;

        // Query the texture for its width and height
        float w, h;
        Helper::SDL_QueryTexture(texture, &w, &h);
        dstrect.w = static_cast<float>(w);
        dstrect.h = static_cast<float>(h);

        Helper::SDL_RenderCopy(Renderer::getRenderer(), texture, nullptr, &dstrect);
    }
    textDrawQueueHUD.clear();
}

void Engine::renderOutro(int index) {

    std::array<SDL_Texture *, 2> outroImageCache = ImageDB::getOutroImageCache();

    if (index == 0) {
        imageDrawRequest request;

        request.texture = outroImageCache[0];

        if (outroImageCache[0] == nullptr) {
            exit(0);
        }

        SDL_FRect dstrect;
        dstrect.x       = 0;
        dstrect.y       = 0;
        dstrect.w       = Engine::getXResolution();
        dstrect.h       = Engine::getYResolution();
        request.dstrect = dstrect;

        SDL_FRect *srcrect = nullptr;
        request.srcrect    = srcrect;

        imageDrawQueue.push_back(request);
    } else {
        imageDrawRequest request;

        request.texture = outroImageCache[1];

        if (outroImageCache[1] == nullptr) {
            exit(0);
        }

        SDL_FRect dstrect;
        dstrect.x       = 0;
        dstrect.y       = 0;
        dstrect.w       = Engine::getXResolution();
        dstrect.h       = Engine::getYResolution();
        request.dstrect = dstrect;

        SDL_FRect *srcrect = nullptr;
        request.srcrect    = srcrect;

        imageDrawQueue.push_back(request);
    }
}

void Engine::renderActor(Actor &actor) {
    // Render the actor
    // if actor.view is not nullptr, render the actor's view at the actor's position
    imageDrawRequest request;

    request.actor = &actor;

    // check if the actor is not in the cameras view, if not, dont render
    if (actor.getPosition().x + actor.getViewSize().x < getCameraPosition().x - (Engine::getXResolution() / 2) ||
        actor.getPosition().x - actor.getViewSize().x > getCameraPosition().x + (Engine::getXResolution() / 2) ||
        actor.getPosition().y + actor.getViewSize().y < getCameraPosition().y - (Engine::getYResolution() / 2) ||
        actor.getPosition().y - actor.getViewSize().y > getCameraPosition().y + (Engine::getYResolution() / 2)) {
        return;
    }

    imageDrawQueue.push_back(request);
}

bool Engine::findActorInVector(std::vector<Actor *> actors, Actor *actor) {
    for (auto &a : actors) {
        if (a == actor) {
            return true;
        }
    }
    return false;
}

// Returns a cached texture if available, otherwise renders and caches it.
SDL_Texture *Engine::getTextTexture(const std::string &text, int fontSize, const SDL_Color &color) {
    std::string key = generateTextKey(text, fontSize, color);

    auto it = textTextureCache.find(key);
    if (it != textTextureCache.end()) {
        return it->second; //
    }

    // Not in cache, so render it.
    TTF_Font    *font    = TextDB::getFont(); // or however you retrieve the appropriate font
    SDL_Surface *surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) {
        std::cout << "Failed to render text surface: " << TTF_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(Renderer::getRenderer(), surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        std::cout << "Failed to create text texture: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    // Cache the texture
    textTextureCache[key] = texture;
    return texture;
}

void Engine::clearTextTextureCache() {
    for (auto &pair : textTextureCache) {
        SDL_DestroyTexture(pair.second);
    }
    textTextureCache.clear();
}

std::string Engine::getGameTitle() {
    return gameTitle;
}

void Engine::setGameTitle(std::string title) {
    gameTitle = title;
}

void Engine::setXResolution(int n) {
    xResolution = n;
}

void Engine::setYResolution(int n) {
    yResolution = n;
}

int Engine::getXResolution() {
    return xResolution;
}

int Engine::getYResolution() {
    return yResolution;
}

void Engine::setClearedColor(int r, int g, int b) {
    clear_color_r = r;
    clear_color_g = g;
    clear_color_b = b;
}

int Engine::getClearedColorR() {
    return clear_color_r;
}

int Engine::getClearedColorG() {
    return clear_color_g;
}

int Engine::getClearedColorB() {
    return clear_color_b;
}

void Engine::addActor(Actor &actor) {
    masterActorList.push_back(actor);
}

int Engine::getGameState() {
    return gameState;
}

void Engine::setGameState(int state) {
    gameState = state;
}

void Engine::addTextureToCache(std::string name, SDL_Texture *texture) {
    textureCache[name] = texture;
}

void Engine::setCameraPosition(glm::vec2 position) {
    cameraPosition = position;
}

void Engine::setCenterX(int x) {
    centerX = x;
}

void Engine::setCenterY(int y) {
    centerY = y;
}

int Engine::getCenterX() {
    return centerX;
}

int Engine::getCenterY() {
    return centerY;
}

void Engine::setHasPlayer(bool b) {
    hasPlayer = b;
}

bool Engine::getHasPlayer() {
    return hasPlayer;
}

glm::vec2 Engine::getCameraPosition() {
    return cameraPosition;
}

void Engine::setZoomFactor(float scale) {
    zoomFactor = scale;
}

float Engine::getZoomFactor() {
    return zoomFactor;
}

void Engine::setCamEaseFactor(float cam_ease_factor) {
    camEaseFactor = cam_ease_factor;
}

float Engine::getCamEaseFactor() {
    return camEaseFactor;
}

void Engine::setPlayerSpeed(float speed) {
    playerSpeed = speed;
}

float Engine::getPlayerSpeed() {
    return playerSpeed;
}

luabridge::LuaRef Engine::Find(const std::string &name) {
    for (auto &actor : masterActorList) {
        if (actor.getName() == name) {
            // need to return the refreence to the  actor
            return luabridge::LuaRef(ComponentManager::getLuaState(), &actor);
        }
    }
    return luabridge::LuaRef(ComponentManager::getLuaState());
}

luabridge::LuaRef Engine::FindAll(const std::string &name) {
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