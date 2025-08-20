#pragma once

#include "SDL2/SDL.h"
#include "SDL2_ttf/SDL_ttf.h"

#include <string>
#include <vector>

class TextDB {
  public:
    static void check();

    // Text.Draw(str_content, x, y, font_name, font_size, r, g, b, a);
    static void Draw(const std::string &text, float x, float y, const std::string &fontName, int fontSize, int r, int g, int b, int a);

    static TTF_Font *getFont(const std::string &fontName, int fontSize);
    static void      addFontToCache(const std::string &fontName, int fontSize, TTF_Font *font);

  private:
    inline static std::unordered_map<std::string, std::unordered_map<int, TTF_Font *>> fontCache; // Cache for fonts by name and size
};
