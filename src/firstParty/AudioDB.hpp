#pragma once

#include "AudioHelper.h"
#include "SDL2_mixer/SDL_mixer.h"

#include <array>
#include <string>
#include <unordered_map>

class AudioDB {
  public:
    static void init();
    static void Play(int channel, const std::string &clipName, bool doesLoop);
    static void Halt(int channel);
    static void SetVolume(int channel, float volume);

    static Mix_Chunk *getIntroBGM();

    static Mix_Chunk *getAudio(std::string audioName);

    static std::array<Mix_Chunk *, 2> getOutroAudioCache();

    static std::string getScoreSFXPath();

    static void loadSoundEffect(std::string audioName);

  private:
    inline static std::string introBGMPath;
    inline static Mix_Chunk  *introBGM;

    inline static std::array<Mix_Chunk *, 2> outroAudioCache = {nullptr, nullptr}; // [0] = game over, [1] = win

    inline static std::unordered_map<std::string, Mix_Chunk *> audioCache;

    inline static std::string scoreSFX;
};
