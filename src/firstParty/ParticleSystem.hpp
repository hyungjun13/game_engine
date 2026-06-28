#pragma once

#include <queue>
#include <string>
#include <vector>

#include "Helper.h"

class Actor;

class ParticleSystem {
  public:
    ParticleSystem()  = default;
    ~ParticleSystem() = default;

    void OnStart();
    void OnUpdate();
    void OnLateUpdate() {}
    void OnDestroy() {}

    // Suite #3 : runtime Lua control
    void Stop();
    void Play();
    void Burst();

    // Component metadata
    std::string key        = "";
    std::string type       = "ParticleSystem";
    bool        enabled    = true;
    bool        hasStarted = false;
    Actor      *actor      = nullptr;

    // Suite #0 / #1 static properties
    float       x                     = 0.0f;
    float       y                     = 0.0f;
    int         frames_between_bursts = 1;
    int         burst_quantity        = 1;
    float       start_scale_min       = 1.0f;
    float       start_scale_max       = 1.0f;
    float       rotation_min          = 0.0f;
    float       rotation_max          = 0.0f;
    int         start_color_r         = 255;
    int         start_color_g         = 255;
    int         start_color_b         = 255;
    int         start_color_a         = 255;
    float       emit_radius_min       = 0.0f;
    float       emit_radius_max       = 0.5f;
    float       emit_angle_min        = 0.0f;
    float       emit_angle_max        = 360.0f;
    std::string image                 = "";
    int         sorting_order         = 9999;

    // Suite #2 animated properties
    int   duration_frames     = 300;
    float start_speed_min     = 0.0f;
    float start_speed_max     = 0.0f;
    float rotation_speed_min  = 0.0f;
    float rotation_speed_max  = 0.0f;
    float gravity_scale_x     = 0.0f;
    float gravity_scale_y     = 0.0f;
    float drag_factor         = 1.0f;
    float angular_drag_factor = 1.0f;
    // -1 sentinel means "not configured" (no lerp applied)
    float end_scale   = -1.0f;
    int   end_color_r = -1;
    int   end_color_g = -1;
    int   end_color_b = -1;
    int   end_color_a = -1;

  private:
    // DOD particle arrays (one entry per particle slot)
    std::vector<bool>  is_active;
    std::vector<int>   start_frame;
    std::vector<float> x_pos;
    std::vector<float> y_pos;
    std::vector<float> x_vel;
    std::vector<float> y_vel;
    std::vector<float> rotation_vals;
    std::vector<float> rotation_speed_vals;
    std::vector<float> initial_scale;
    std::vector<int>   initial_r;
    std::vector<int>   initial_g;
    std::vector<int>   initial_b;
    std::vector<int>   initial_a;

    int             particle_system_frame_number = 0;
    int             number_of_particle_slots     = 0;
    std::queue<int> free_list;
    bool            emission_allowed = true;

    static const std::string DEFAULT_TEXTURE_KEY;

    RandomEngine emit_angle_distribution;
    RandomEngine emit_radius_distribution;
    RandomEngine rotation_distribution;
    RandomEngine scale_distribution;
    RandomEngine speed_distribution;
    RandomEngine rotation_speed_distribution;

    void        GenerateNewParticles(int count);
    std::string GetTextureName() const;
};
