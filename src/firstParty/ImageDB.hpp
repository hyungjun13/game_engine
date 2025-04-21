#pragma once

#include "SDL2/SDL.h"
#include "SDL_image/SDL_image.h"

#include <array>
#include <string>
#include <vector>

class ImageDB {
  public:
    static void         check();
    static void         loadIntroImages();
    static void         loadOutroImages(); // Only need two, one for game over and one for win
    static SDL_Texture *getImage(std::string imageName);

    static std::vector<SDL_Texture *> &getIntroImageCache() {
        return introImageCache;
    }

    static std::array<SDL_Texture *, 2> &getOutroImageCache() {
        return outroImageCache;
    }

    static void loadHUD();

  private:
    inline static std::vector<SDL_Texture *>   introImageCache;
    inline static std::array<SDL_Texture *, 2> outroImageCache = {nullptr, nullptr}; // [0] = game over, [1] = win
};