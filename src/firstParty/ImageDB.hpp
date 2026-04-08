#pragma once

#include "SDL2/SDL.h"
#include "SDL_image/SDL_image.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

struct ImageDrawRequest {
    std::string image_name;
    float       x;
    float       y;
    int         rotation_degrees;
    float       scale_x;
    float       scale_y;
    float       pivot_x;
    float       pivot_y;
    int         r;
    int         g;
    int         b;
    int         a;
    int         sorting_order;
};

class ImageDB {
  public:
    static void         check();
    static void         loadIntroImages();
    static void         loadOutroImages(); // Only need two, one for game over and one for win
    static SDL_Texture *getImage(std::string imageName);
    static void         Draw(const std::string &imageName,
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
                             float              sortingOrder);
    static void         DrawUI(const std::string &imageName,
                               float              x,
                               float              y);
    static void         DrawUIEx(const std::string &imageName,
                                 float              x,
                                 float              y,
                                 float              r,
                                 float              g,
                                 float              b,
                                 float              a,
                                 float              sortingOrder);
    static void         RenderAndClearAllImages();

    static std::vector<SDL_Texture *> &getIntroImageCache();

    static std::array<SDL_Texture *, 2> &getOutroImageCache();

    static void loadHUD();

  private:
    inline static std::vector<SDL_Texture *>                     introImageCache;
    inline static std::array<SDL_Texture *, 2>                   outroImageCache = {nullptr, nullptr}; // [0] = game over, [1] = win
    inline static std::unordered_map<std::string, SDL_Texture *> imageCache;
    inline static std::vector<ImageDrawRequest>                  imageDrawQueue;
};