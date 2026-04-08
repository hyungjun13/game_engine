#include "AudioDB.hpp"
#include "AudioHelper.h"
#include "EngineUtil.hpp"
#include "rapidjson/document.h"

#include <filesystem>
#include <iostream>

Mix_Chunk *AudioDB::getIntroBGM() {
    return introBGM;
}

Mix_Chunk *AudioDB::getAudio(std::string audioName) {
    auto it = audioCache.find(audioName);
    if (it == audioCache.end()) {
        return nullptr;
    }
    return it->second;
}

std::array<Mix_Chunk *, 2> AudioDB::getOutroAudioCache() {
    return outroAudioCache;
}

std::string AudioDB::getScoreSFXPath() {
    return scoreSFX;
}

void AudioDB::loadSoundEffect(std::string audioName) {
    if (audioCache.find(audioName) == audioCache.end()) {
        if (!std::filesystem::exists("resources/audio/" + audioName + ".wav") && !std::filesystem::exists("resources/audio/" + audioName + ".ogg")) {
            std::cout << "error: failed to play audio clip " << audioName;
            exit(0);
        }

        if (std::filesystem::exists("resources/audio/" + audioName + ".wav")) {
            audioCache[audioName] = AudioHelper::Mix_LoadWAV(("resources/audio/" + audioName + ".wav").c_str());
        } else {
            audioCache[audioName] = AudioHelper::Mix_LoadWAV(("resources/audio/" + audioName + ".ogg").c_str());
        }
    }
}

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

void AudioDB::Play(int channel, const std::string &clipName, bool doesLoop) {
    loadSoundEffect(clipName);
    Mix_Chunk *chunk = getAudio(clipName);
    if (chunk == nullptr) {
        return;
    }
    AudioHelper::Mix_PlayChannel(channel, chunk, doesLoop ? -1 : 0);
}

void AudioDB::Halt(int channel) {
    AudioHelper::Mix_HaltChannel(channel);
}

void AudioDB::SetVolume(int channel, float volume) {
    int volumeInt = static_cast<int>(volume);
    AudioHelper::Mix_Volume(channel, volumeInt);
}