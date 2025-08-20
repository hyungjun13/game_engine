#include "TextDB.hpp"
#include "EngineUtil.hpp"
#include "SDL2_ttf/SDL_ttf.h"
#include "rapidjson/document.h"

#include <filesystem>

void TextDB::addFontToCache(const std::string &fontName, int fontSize, TTF_Font *font) {
    fontCache[fontName][fontSize] = font;
}

TTF_Font *TextDB::getFont(const std::string &fontName, int fontSize) {
    // Check if the font is already in the cache
    if (fontCache.find(fontName) != fontCache.end() && fontCache[fontName].find(fontSize) != fontCache[fontName].end()) {
        return fontCache[fontName][fontSize];
    }

    // If not in cache, load the font and add it to the cache
    TTF_Font *newFont = TTF_OpenFont(("resources/fonts/" + fontName + ".ttf").c_str(), fontSize);
    if (!newFont) {
        std::cout << "error: failed to load font " << fontName << " with size " << fontSize << ": " << TTF_GetError() << std::endl;
        return nullptr;
    } //
    addFontToCache(fontName, fontSize, newFont);
    return newFont;
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
}

void TextDB::Draw(const std::string &text, float x, float y, const std::string &fontName, int fontSize, int r, int g, int b, int a) {
    // send requests into the textDrawQueue
    textRequest request;
    request.text     = text;
    request.x        = x;
    request.y        = y;
    request.fontName = fontName;
    request.fontSize = fontSize;
    request.color    = {static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b), static_cast<Uint8>(a)};
    request.alpha    = a;
    Engine::addText(request);
}