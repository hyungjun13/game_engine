#pragma once

#include "SDL2/SDL.h"
#include "SDL2_ttf/SDL_ttf.h"

#include <string>
#include <unordered_map>
#include <vector>

class TextDB {
  public:
    static void      check();
    static void      loadIntroText();
    static TTF_Font *GetFontByNameSize(const std::string &fontName, int fontSize);
    static void      Draw(const std::string &strContent,
                          float              x,
                          float              y,
                          const std::string &fontName,
                          float              fontSize,
                          float              r,
                          float              g,
                          float              b,
                          float              a);

    static std::vector<std::string> &getIntroTextCache();

    static void setFont(TTF_Font *f);

    static TTF_Font *getFont();

  private:
    inline static std::unordered_map<std::string, std::unordered_map<int, TTF_Font *>> fontCache;
    inline static std::vector<std::string>                                             introTextCache;

    inline static TTF_Font *font = nullptr;
};
