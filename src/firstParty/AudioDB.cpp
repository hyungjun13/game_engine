#include "AudioDB.hpp"
#include "AudioHelper.h"
#include "EngineUtil.hpp"
#include "rapidjson/document.h"

#include <filesystem>

void AudioDB::init() {
    // read the game.config file

    AudioHelper::Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    rapidjson::Document document;

    EngineUtil::ReadJsonFile("resources/game.config", document);

    if (document.HasMember("intro_bgm")) {

        // need to check if either .wav or .ogg exists
        std::string introBGMPath = document["intro_bgm"].GetString();
        if (!std::filesystem::exists("resources/audio/" + introBGMPath + ".wav") && !std::filesystem::exists("resources/audio/" + introBGMPath + ".ogg")) {
            std::cout << "error: failed to play audio clip " << introBGMPath;
            exit(0);
        }

        if (std::filesystem::exists("resources/audio/" + introBGMPath + ".wav")) {
            introBGM = AudioHelper::Mix_LoadWAV(("resources/audio/" + introBGMPath + ".wav").c_str());
        } else {
            introBGM = AudioHelper::Mix_LoadWAV(("resources/audio/" + introBGMPath + ".ogg").c_str());
        }
    }

    if (document.HasMember("score_sfx")) {
        std::string scoreSFXPath = document["score_sfx"].GetString();
        if (!std::filesystem::exists("resources/audio/" + scoreSFXPath + ".wav") && !std::filesystem::exists("resources/audio/" + scoreSFXPath + ".ogg")) {
            std::cout << "error: failed to play audio clip " << scoreSFXPath;
            exit(0);
        }

        if (std::filesystem::exists("resources/audio/" + scoreSFXPath + ".wav")) {
            audioCache[scoreSFXPath] = AudioHelper::Mix_LoadWAV(("resources/audio/" + scoreSFXPath + ".wav").c_str());
            scoreSFX                 = scoreSFXPath;
        } else {
            audioCache[scoreSFXPath] = AudioHelper::Mix_LoadWAV(("resources/audio/" + scoreSFXPath + ".ogg").c_str());
            scoreSFX                 = scoreSFXPath;
        }
    }
}