#pragma once

#include "SDL2/SDL.h"
#include "SDL2_ttf/SDL_ttf.h"

#include <string>
#include <vector>

class TextDB {
  public:
    static void check();
    static void loadIntroText();

    inline static std::string getIntroText(int index);

    inline static std::vector<std::string> &getIntroTextCache() {
        return introTextCache;
    }

    inline static void setFont(TTF_Font *f) {
        font = f;
    }

    inline static TTF_Font *getFont() {
        return font;
    }

  private:
    inline static std::vector<std::string> introTextCache;

    inline static TTF_Font *font = nullptr;
};
