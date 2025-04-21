#include "Input.hpp"
#include "SDL2/SDL.h"
#include <algorithm>

// Initialize states
void Input::Init() {
    keyboard_states.clear();
    just_became_down_scancodes.clear();
    just_became_up_scancodes.clear();

    mouse_button_states.clear();
    just_became_down_mouse_buttons.clear();
    just_became_up_mouse_buttons.clear();
}

// Process each event
void Input::ProcessEvent(const SDL_Event &e) {
    // Process keyboard events
    if (e.type == SDL_KEYDOWN) { // only process the first keydown
        SDL_Scancode code = e.key.keysym.scancode;
        // Only mark as just became down if it was not already down.
        if (keyboard_states[code] == INPUT_STATE_UP ||
            keyboard_states[code] == INPUT_STATE_JUST_BECAME_UP) {
            keyboard_states[code] = INPUT_STATE_JUST_BECAME_DOWN;
            just_became_down_scancodes.push_back(code);
        }
    }

    if (e.type == SDL_KEYUP) {
        SDL_Scancode code     = e.key.keysym.scancode;
        keyboard_states[code] = INPUT_STATE_JUST_BECAME_UP;
        just_became_up_scancodes.push_back(code);
    }

    // Process mouse events
    if (e.type == SDL_MOUSEBUTTONDOWN) {
        Uint8 button = e.button.button;
        if (mouse_button_states[button] == INPUT_STATE_UP ||
            mouse_button_states[button] == INPUT_STATE_JUST_BECAME_UP) {
            mouse_button_states[button] = INPUT_STATE_JUST_BECAME_DOWN;
            just_became_down_mouse_buttons.push_back(button);
        }
    }
    if (e.type == SDL_MOUSEBUTTONUP) {
        Uint8 button                = e.button.button;
        mouse_button_states[button] = INPUT_STATE_JUST_BECAME_UP;
        just_became_up_mouse_buttons.push_back(button);
    }

    if (e.type == SDL_MOUSEMOTION) {
        // Update the mouse p osition using t he event's motion values.
        mouse_position = glm::vec2(e.motion.x, e.motion.y);
    }

    if (e.type == SDL_MOUSEWHEEL) {
        // Update the mouse scroll delta using the event's y value.
        mouse_scroll_this_frame = e.wheel.preciseY;
    }

    if (e.type == SDL_QUIT) {
        isQuit = true;
    }
}

// At the end of the frame, update states
void Input::LateUpdate() {
    // Keyboard updates
    for (auto code : just_became_down_scancodes) {
        keyboard_states[code] = INPUT_STATE_DOWN;
    }
    just_became_down_scancodes.clear();

    for (auto code : just_became_up_scancodes) {
        keyboard_states[code] = INPUT_STATE_UP;
    }
    just_became_up_scancodes.clear();

    // Mouse button updates
    for (auto button : just_became_down_mouse_buttons) {
        mouse_button_states[button] = INPUT_STATE_DOWN;
    }
    just_became_down_mouse_buttons.clear();

    for (auto button : just_became_up_mouse_buttons) {
        mouse_button_states[button] = INPUT_STATE_UP;
    }
    just_became_up_mouse_buttons.clear();

    mouse_scroll_this_frame = 0.0f; // Rese scroll  delta for the next frame
}

// Keyboard getters
bool Input::GetKey(std::string keycode) {
    auto it = __keycode_to_scancode.find(keycode);
    if (it != __keycode_to_scancode.end()) {
        return keyboard_states[it->second] == INPUT_STATE_DOWN ||
               keyboard_states[it->second] == INPUT_STATE_JUST_BECAME_DOWN;
    }

    return false;
}

bool Input::GetKeyDown(std::string keycode) {
    // return keyboard_states[keycode] == INPUT_STATE_JUST_BECAME_DOWN;

    auto it = __keycode_to_scancode.find(keycode);
    if (it != __keycode_to_scancode.end()) {
        return keyboard_states[it->second] == INPUT_STATE_JUST_BECAME_DOWN;
    }
    return false;
}

bool Input::GetKeyUp(std::string keycode) {
    // return keyboard_states[keycode] == INPUT_STATE_JUST_BECAME_UP;

    auto it = __keycode_to_scancode.find(keycode);
    if (it != __keycode_to_scancode.end()) {
        return keyboard_states[it->second] == INPUT_STATE_JUST_BECAME_UP; //
    }
    return false;
}

// Mouse getter
bool Input::GetMouseButton(Uint8 button) {
    return mouse_button_states[button] == INPUT_STATE_DOWN ||
           mouse_button_states[button] == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetMouseButtonDown(Uint8 button) {
    return mouse_button_states[button] == INPUT_STATE_JUST_BECAME_DOWN;
}

bool Input::GetMouseButtonUp(Uint8 button) {
    return mouse_button_states[button] == INPUT_STATE_JUST_BECAME_UP;
}

bool Input::GetQuit() {
    // check if SDL_QUIT event is in the event queue
    return isQuit;
}

glm::vec2 Input::GetMousePosition() {
    /*Use SDL_MOUSEMOTION events as they come through the event queue.
    Your C++ code should return a glm::vec2 to Lua.
    Be sure to expose the “x” and “y” properties of the C++ struct to Lua, and its name in Lua to vec2. See the example here if you’re stuck
    */
    return mouse_position;
}

float Input::GetMouseScrollDelta() {
    return mouse_scroll_this_frame;
}

void Input::ShowCursor() {
    SDL_ShowCursor(SDL_ENABLE);
}

void Input::HideCursor() {
    SDL_ShowCursor(SDL_DISABLE);
}