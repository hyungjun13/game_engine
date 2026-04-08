#include "TextDB.hpp"
#include "Engine.hpp"
#include "EngineUtil.hpp"
#include "SDL2_ttf/SDL_ttf.h"
#include "rapidjson/document.h"

#include <filesystem>
#include <iostream>

std::vector<std::string> &TextDB::getIntroTextCache() {
    return introTextCache;
}

void TextDB::setFont(TTF_Font *f) {
    font = f;
}

TTF_Font *TextDB::getFont() {
    return font;
}

void TextDB::check() {
    TTF_Init();
    // If no intro_image is defined, do not render or process any intro_text
    if (!std::filesystem::exists("resources/game.config")) {
        return;
    }

    // read the game.config file
    rapidjson::Document document;
    EngineUtil::ReadJsonFile("resources/game.config", document);

    if (document.HasMember("font")) {
        std::string fontName = document["font"].GetString();
        if (!std::filesystem::exists("resources/fonts/" + fontName + ".ttf")) {
            std::cout << "error: font " << fontName << " missing";
            exit(0);
        }
    } else if (document.HasMember("intro_text") && !document.HasMember("font")) {
        std::cout << "error: text render failed. No font configured";
        exit(0);
    }

    // Load the font
    if (document.HasMember("font")) {
        std::string fontName = document["font"].GetString();
        font                 = TTF_OpenFont(("resources/fonts/" + fontName + ".ttf").c_str(), 16);
    }
}

void TextDB::loadIntroText() {
    /*
    All fonts will be truetype fonts (with .ttf extension)
A font named example refers to resources/fonts/example.ttf
All fonts render with a fontsize of 16 and color of {255, 255, 255, 255}
All fonts will render with the TTF_RenderText_Solid style.
All text will render at (25, height-50) where  height is the window height.

    */

    rapidjson::Document document;
    EngineUtil::ReadJsonFile("resources/game.config", document);

    if (document.HasMember("intro_text")) {

        for (auto &text : document["intro_text"].GetArray()) {
            textRequest request;
            request.text     = text.GetString();
            request.fontSize = 16;
            request.color    = {255, 255, 255, 255};
            request.x        = 25;
            request.y        = Engine::getYResolution() - 50;

            introTextCache.push_back(request.text);
        }
    }
}

TTF_Font *TextDB::GetFontByNameSize(const std::string &fontName, int fontSize) {
    auto fontIt = fontCache.find(fontName);
    if (fontIt != fontCache.end()) {
        auto sizeIt = fontIt->second.find(fontSize);
        if (sizeIt != fontIt->second.end()) {
            return sizeIt->second;
        }
    }

    std::string fontPath = "resources/fonts/" + fontName + ".ttf";
    if (!std::filesystem::exists(fontPath)) {
        std::cout << "error: font " << fontName << " missing";
        exit(0);
    }

    TTF_Font *loadedFont = TTF_OpenFont(fontPath.c_str(), fontSize);
    if (!loadedFont) {
        std::cout << "error: font " << fontName << " missing";
        exit(0);
    }

    fontCache[fontName][fontSize] = loadedFont;
    return loadedFont;
}

void TextDB::Draw(const std::string &strContent,
                  float              x,
                  float              y,
                  const std::string &fontName,
                  float              fontSize,
                  float              r,
                  float              g,
                  float              b,
                  float              a) {
    int       x_i        = static_cast<int>(x);
    int       y_i        = static_cast<int>(y);
    int       fontSize_i = static_cast<int>(fontSize);
    int       r_i        = static_cast<int>(r);
    int       g_i        = static_cast<int>(g);
    int       b_i        = static_cast<int>(b);
    int       a_i        = static_cast<int>(a);
    SDL_Color color      = {static_cast<Uint8>(r_i), static_cast<Uint8>(g_i), static_cast<Uint8>(b_i), static_cast<Uint8>(a_i)};

    Engine::QueueTextDraw(strContent, x_i, y_i, fontName, fontSize_i, color);
}