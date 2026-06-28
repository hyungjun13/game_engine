#include "ParticleSystem.hpp"

#include <glm/glm.hpp>

#include "ImageDB.hpp"

const std::string ParticleSystem::DEFAULT_TEXTURE_KEY = "__particle_default__";

void ParticleSystem::OnStart() {
    // Enforce minimum values per spec
    if (frames_between_bursts < 1) frames_between_bursts = 1;
    if (burst_quantity < 1)        burst_quantity        = 1;
    if (duration_frames < 1)       duration_frames       = 1;

    // Initialize RandomEngines exactly once (seeds from assignment spec)
    emit_angle_distribution     = RandomEngine(emit_angle_min, emit_angle_max, 298);
    emit_radius_distribution    = RandomEngine(emit_radius_min, emit_radius_max, 404);
    rotation_distribution       = RandomEngine(rotation_min, rotation_max, 440);
    scale_distribution          = RandomEngine(start_scale_min, start_scale_max, 494);
    speed_distribution          = RandomEngine(start_speed_min, start_speed_max, 498);
    rotation_speed_distribution = RandomEngine(rotation_speed_min, rotation_speed_max, 305);

    // Ensure the default particle texture is cached
    ImageDB::CreateDefaultParticleTextureWithName(DEFAULT_TEXTURE_KEY);
}

std::string ParticleSystem::GetTextureName() const {
    return image.empty() ? DEFAULT_TEXTURE_KEY : image;
}

void ParticleSystem::GenerateNewParticles(int count) {
    for (int i = 0; i < count; ++i) {
        int slot;
        if (!free_list.empty()) {
            slot = free_list.front();
            free_list.pop();
        } else {
            slot = number_of_particle_slots++;
            is_active.push_back(false);
            start_frame.push_back(0);
            x_pos.push_back(0.0f);
            y_pos.push_back(0.0f);
            x_vel.push_back(0.0f);
            y_vel.push_back(0.0f);
            rotation_vals.push_back(0.0f);
            rotation_speed_vals.push_back(0.0f);
            initial_scale.push_back(1.0f);
            initial_r.push_back(255);
            initial_g.push_back(255);
            initial_b.push_back(255);
            initial_a.push_back(255);
        }

        // Emission shape via polar coordinates
        float angle_radians = glm::radians(emit_angle_distribution.Sample());
        float radius        = emit_radius_distribution.Sample();
        float cos_angle     = glm::cos(angle_radians);
        float sin_angle     = glm::sin(angle_radians);

        // Emission velocity (same angle as position offset)
        float speed = speed_distribution.Sample();

        is_active[slot]           = true;
        start_frame[slot]         = particle_system_frame_number;
        x_pos[slot]               = x + cos_angle * radius;
        y_pos[slot]               = y + sin_angle * radius;
        x_vel[slot]               = cos_angle * speed;
        y_vel[slot]               = sin_angle * speed;
        rotation_vals[slot]       = rotation_distribution.Sample();
        rotation_speed_vals[slot] = rotation_speed_distribution.Sample();
        initial_scale[slot]       = scale_distribution.Sample();
        initial_r[slot]           = start_color_r;
        initial_g[slot]           = start_color_g;
        initial_b[slot]           = start_color_b;
        initial_a[slot]           = start_color_a;
    }
}

void ParticleSystem::OnUpdate() {
    // Burst at top of update if it's the right frame and emission is allowed
    if (particle_system_frame_number % frames_between_bursts == 0 && emission_allowed) {
        GenerateNewParticles(burst_quantity);
    }

    const std::string tex_name = GetTextureName();

    for (int i = 0; i < number_of_particle_slots; ++i) {
        if (!is_active[i]) continue;

        // Lifetime check
        int frames_alive = particle_system_frame_number - start_frame[i];
        if (frames_alive >= duration_frames) {
            is_active[i] = false;
            free_list.push(i);
            continue;
        }

        // Apply gravity to velocity
        x_vel[i] += gravity_scale_x;
        y_vel[i] += gravity_scale_y;

        // Apply drag to velocity and angular drag to angular velocity
        x_vel[i]               *= drag_factor;
        y_vel[i]               *= drag_factor;
        rotation_speed_vals[i] *= angular_drag_factor;

        // Apply velocities to position and rotation
        x_pos[i]         += x_vel[i];
        y_pos[i]         += y_vel[i];
        rotation_vals[i] += rotation_speed_vals[i];

        // Compute lifetime progress for lerps
        float lifetime_progress = static_cast<float>(frames_alive) / static_cast<float>(duration_frames);

        // Scale interpolation
        float current_scale = initial_scale[i];
        if (end_scale >= 0.0f) {
            current_scale = glm::mix(initial_scale[i], end_scale, lifetime_progress);
        }

        // Color interpolation (per channel, only if end value is configured)
        float current_r = static_cast<float>(initial_r[i]);
        float current_g = static_cast<float>(initial_g[i]);
        float current_b = static_cast<float>(initial_b[i]);
        float current_a = static_cast<float>(initial_a[i]);
        if (end_color_r >= 0) {
            current_r = glm::mix(static_cast<float>(initial_r[i]), static_cast<float>(end_color_r), lifetime_progress);
        }
        if (end_color_g >= 0) {
            current_g = glm::mix(static_cast<float>(initial_g[i]), static_cast<float>(end_color_g), lifetime_progress);
        }
        if (end_color_b >= 0) {
            current_b = glm::mix(static_cast<float>(initial_b[i]), static_cast<float>(end_color_b), lifetime_progress);
        }
        if (end_color_a >= 0) {
            current_a = glm::mix(static_cast<float>(initial_a[i]), static_cast<float>(end_color_a), lifetime_progress);
        }

        // Render at scene coordinates with pivot 0.5, 0.5
        ImageDB::Draw(
            tex_name,
            x_pos[i],
            y_pos[i],
            rotation_vals[i],
            current_scale,
            current_scale,
            0.5f,
            0.5f,
            current_r,
            current_g,
            current_b,
            current_a,
            static_cast<float>(sorting_order));
    }

    particle_system_frame_number++;
}

void ParticleSystem::Stop() {
    emission_allowed = false;
}

void ParticleSystem::Play() {
    emission_allowed = true;
}

void ParticleSystem::Burst() {
    GenerateNewParticles(burst_quantity);
}
