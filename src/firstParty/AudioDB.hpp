#pragma once

#include "AudioHelper.h"
#include "SDL2_mixer/SDL_mixer.h"
#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

class AudioDB {
  public:
    static void init();

    static inline Mix_Chunk *getIntroBGM() {
        return introBGM;
    }

    static inline Mix_Chunk *getAudio(std::string audioName) {
        return audioCache[audioName];
    }

    static inline std::array<Mix_Chunk *, 2> getOutroAudioCache() {
        return outroAudioCache;
    }

    static inline std::string getScoreSFXPath() {
        return scoreSFX;
    }

    static inline void loadSoundEffect(std::string audioName) {
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

  private:
    inline static std::string introBGMPath;
    inline static Mix_Chunk  *introBGM;

    inline static std::array<Mix_Chunk *, 2> outroAudioCache = {nullptr, nullptr}; // [0] = game over, [1] = win

    inline static std::unordered_map<std::string, Mix_Chunk *> audioCache;

    inline static std::string scoreSFX;
};
