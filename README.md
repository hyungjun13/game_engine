# 2D Game Engine

A Unity-inspired, Lua-scripted 2D game engine built in C++.  
Developed at the **University of Michigan** as part of **EECS 498: Game Engine Architecture**.

---

![agame_engine_k4E7uA6kG6](https://github.com/user-attachments/assets/d59aa408-033f-4750-923e-d7af300a1507)

---

## Overview

Phantom Engine is a from-scratch 2D game engine that pairs a high-performance C++ core with a Lua scripting layer. The architecture mirrors Unity's component-actor model — actors are gameobjects, components are scripts that attach to them, and the engine drives the standard `OnStart` → `OnUpdate` → `OnLateUpdate` lifecycle each frame.

The engine was built incrementally across a semester-long project, adding physics, particles, audio, and scene management as discrete milestones.

---

## Features

| System                    | Description                                                                                                                                                                                  |
| ------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Component-Actor Model** | Actors hold any number of Lua or C++ components. Components communicate via `actor:GetComponent()`.                                                                                          |
| **Lua Scripting**         | Full gameplay logic in Lua. The engine calls `OnStart`, `OnUpdate`, `OnLateUpdate`, `OnDestroy`, and all physics callbacks automatically.                                                    |
| **Box2D Physics**         | Rigidbodies with dynamic, static, and kinematic body types. Box and circle colliders. Trigger volumes. Raycasting.                                                                           |
| **Particle System**       | Data-oriented (DOD) particle system with polar emission, per-particle gravity/drag/angular drag, scale/color lerp over lifetime, and a free-list allocator for zero-allocation steady state. |
| **Image Rendering**       | World-space and HUD-space image drawing with sorting order, per-pixel color modulation, and arbitrary pivot points.                                                                          |
| **Text Rendering**        | TTF text rendering with per-frame caching — textures are created once and reused until the string or style changes.                                                                          |
| **Audio**                 | SDL2 Mixer-backed audio with channel-based play/halt and runtime volume control.                                                                                                             |
| **Camera**                | Settable world-space position, zoom, and smoothstep ease factor.                                                                                                                             |
| **Scene System**          | JSON-defined scenes. Load/unload at runtime; `DontDestroyOnLoad` for persistent actors.                                                                                                      |
| **Input**                 | Full keyboard and mouse API: held, just-pressed, just-released, cursor position, scroll delta.                                                                                               |
| **Template System**       | Actor templates defined in JSON. `Actor.Instantiate()` clones them at runtime.                                                                                                               |

---

## Architecture

```
┌───────────────────────────────────────────┐
│                 Lua Scripts               │  ← Game code lives here
│   OnStart / OnUpdate / OnLateUpdate …     │
└────────────────┬──────────────────────────┘
                 │  LuaBridge
┌────────────────▼──────────────────────────┐
│              Lua API Namespaces           │
│  Actor · Input · Image · Text · Audio     │
│  Camera · Scene · Physics · Application   │
└────────────────┬──────────────────────────┘
                 │
┌────────────────▼──────────────────────────┐
│                C++ Engine Core            │
│                                           │
│  Engine.cpp        – game loop            │
│  ComponentManager  – script lifecycle     │
│  SceneLoader       – JSON scene parsing   │
│  ImageDB           – texture cache        │
│  TextDB            – font / TTF cache     │
│  AudioDB           – SDL_mixer wrapper    │
│  Rigidbody         – Box2D integration    │
│  ParticleSystem    – DOD particle engine  │
│  Input             – SDL event mapping    │
└───────────────────────────────────────────┘
```

---

## Lua API Reference

### `Actor`

```lua
Actor.Find(name)            -- first Actor with matching name
Actor.FindAll(name)         -- table of all matching Actors
Actor.Instantiate(template) -- spawns a clone of a template
Actor.Destroy(actor)        -- schedules actor for end-of-frame removal

actor:GetName()
actor:GetID()
actor:GetComponent(type)    -- first component of this type
actor:GetComponents(type)   -- table of all components of this type
actor:GetComponentByKey(k)
actor:AddComponent(type)    -- adds at runtime; OnStart fires next frame
actor:RemoveComponent(c)    -- removes at runtime; OnDestroy fires first
```

### `Input`

```lua
Input.GetKey(keycode)             -- held
Input.GetKeyDown(keycode)         -- just pressed this frame
Input.GetKeyUp(keycode)           -- just released this frame
Input.GetMousePosition()          -- vec2 in screen pixels
Input.GetMouseButton(button)      -- 1=left  2=middle  3=right
Input.GetMouseButtonDown(button)
Input.GetMouseButtonUp(button)
Input.GetMouseScrollDelta()       -- float
Input.ShowCursor() / HideCursor()
```

Supported keycode strings: `"a"`–`"z"`, `"0"`–`"9"`, `"up"` `"down"` `"left"` `"right"`, `"space"`, `"return"`, `"escape"`, `"lshift"`, `"lctrl"`, `"lalt"`, and more.

### `Image`

```lua
-- World-space (affected by camera)
Image.Draw(image, x, y, rotation, scale_x, scale_y, pivot_x, pivot_y, r, g, b, a, sorting_order)

-- HUD-space (screen coordinates, ignores camera)
Image.DrawUI(image, x, y, r, g, b, a, sorting_order)

-- Single pixel
Image.DrawPixel(x, y, r, g, b, a)
```

### `Text`

```lua
Text.Draw(str, x, y, font_name, font_size, r, g, b, a)
```

### `Audio`

```lua
Audio.Play(channel, clip_name, loop)   -- loop: true/false
Audio.Halt(channel)
Audio.SetVolume(channel, volume)       -- 0–128
```

### `Camera`

```lua
Camera.SetPosition(x, y)
Camera.GetPosition()     -- vec2
Camera.GetPositionX()
Camera.GetPositionY()
Camera.SetZoom(factor)
Camera.GetZoom()
Camera.SetEase(factor)   -- smoothing factor (0 = instant, 1 = no movement)
Camera.GetEase()
```

### `Scene`

```lua
Scene.Load(name)         -- loads scene by filename (no extension)
Scene.GetCurrent()       -- returns current scene name
Scene.DontDestroy(actor) -- actor survives scene transitions
```

### `Physics`

```lua
-- Returns first hit or nil
local hit = Physics.Raycast(origin, direction, distance)

-- Returns table of all hits
local hits = Physics.RaycastAll(origin, direction, distance)

-- RaycastHit fields:
hit.other              -- Actor
hit.point              -- Vector2 (world position)
hit.normal             -- Vector2 (surface normal)
hit.relative_velocity  -- Vector2
```

### `Application`

```lua
Application.Quit()
Application.Sleep(milliseconds)
Application.OpenURL(url)
Application.GetFrame()   -- current engine frame number (integer)
```

### `Debug`

```lua
Debug.Log(value)   -- prints to stdout
```

---

## Component Lifecycle

Every Lua component table may define any of these functions:

```lua
local MyComponent = {}

function MyComponent:OnStart()         end  -- called once before first update
function MyComponent:OnUpdate()        end  -- called every frame
function MyComponent:OnLateUpdate()    end  -- called after all OnUpdates
function MyComponent:OnDestroy()       end  -- called when component/actor removed

-- Physics (requires a Rigidbody on the same actor)
function MyComponent:OnCollisionEnter(collision) end
function MyComponent:OnCollisionExit(collision)  end
function MyComponent:OnTriggerEnter(collision)   end
function MyComponent:OnTriggerExit(collision)    end

return MyComponent
```

Components are sorted by key within each actor, so execution order is deterministic.

---

## Built-in C++ Components

### `Rigidbody`

```json
{
  "type": "Rigidbody",
  "body_type": "dynamic",
  "collider_type": "box",
  "width": 1.0,
  "height": 1.0,
  "gravity_scale": 1.0,
  "bounciness": 0.3,
  "friction": 0.3,
  "angular_friction": 0.3,
  "has_collider": true,
  "has_trigger": true,
  "trigger_type": "box",
  "trigger_width": 1.0,
  "trigger_height": 1.0
}
```

Runtime API (from Lua):

```lua
local rb = self.actor:GetComponent("Rigidbody")
rb:GetPosition()           -- Vector2
rb:GetVelocity()           -- Vector2
rb:GetAngularVelocity()    -- float (radians/s)
rb:GetUpDirection()        -- Vector2
rb:GetRightDirection()     -- Vector2
rb:SetPosition(vec2)
rb:SetVelocity(vec2)
rb:SetRotation(degrees)
rb:SetGravityScale(scale)
rb:AddForce(vec2)
```

### `ParticleSystem`

A data-oriented particle emitter. Multiple instances can run on the same actor to layer effects (e.g., a fire made of four overlapping particle systems).

```json
{
  "type": "ParticleSystem",
  "image": "smoke",
  "burst_quantity": 3,
  "frames_between_bursts": 1,
  "duration_frames": 80,
  "emit_radius_min": 0.0,
  "emit_radius_max": 0.5,
  "emit_angle_min": 240.0,
  "emit_angle_max": 300.0,
  "start_speed_min": 0.03,
  "start_speed_max": 0.05,
  "start_scale_min": 0.5,
  "start_scale_max": 1.0,
  "end_scale": 0.0,
  "gravity_scale_x": 0.0,
  "gravity_scale_y": -0.001,
  "drag_factor": 0.95,
  "angular_drag_factor": 0.98,
  "rotation_min": 0.0,
  "rotation_max": 360.0,
  "rotation_speed_min": -5.0,
  "rotation_speed_max": 5.0,
  "start_color_r": 255,
  "start_color_g": 100,
  "start_color_b": 50,
  "start_color_a": 200,
  "end_color_r": 255,
  "end_color_g": 200,
  "end_color_b": 50,
  "end_color_a": 0,
  "sorting_order": 5
}
```

Runtime control from Lua:

```lua
local ps = self.actor:GetComponent("ParticleSystem")
ps:Stop()    -- pause emission
ps:Play()    -- resume emission
ps:Burst()   -- emit one burst immediately regardless of frame counter
```

If `image` is omitted, the engine uses a built-in 8×8 white square texture.

---

## Project Layout

```
resources/              ← active game project
├── game.config         ← title, initial scene
├── rendering.config    ← resolution, clear color
├── scenes/             ← JSON scene files (.scene)
├── actor_templates/    ← reusable actor JSON templates (.template)
├── component_types/    ← Lua component scripts (.lua)
├── images/             ← PNG/JPG assets (referenced by filename without extension)
├── fonts/              ← TTF font files
└── audio/              ← WAV / OGG audio clips

src/firstParty/         ← engine source
├── Engine.*            ← game loop, rendering pipeline, actor lifecycle
├── ComponentManager.*  ← LuaBridge bindings, component scheduling
├── Actor.*             ← actor and component storage
├── Rigidbody.*         ← Box2D body wrapper
├── ParticleSystem.*    ← data-oriented particle engine
├── ImageDB.*           ← texture cache and draw queue
├── TextDB.*            ← font cache, TTF rendering
├── AudioDB.*           ← SDL_mixer wrapper
├── Input.*             ← keyboard/mouse state machine
├── SceneLoader.*       ← JSON scene parser
└── TemplateDB.*        ← actor template cache
```

---

## Configuration

**`game.config`**

```json
{
  "game_title": "My Game",
  "initial_scene": "main"
}
```

**`rendering.config`**

```json
{
  "x_resolution": 1280,
  "y_resolution": 720,
  "clear_color_r": 20,
  "clear_color_g": 20,
  "clear_color_b": 30
}
```

---

## Building

**Requirements:** CMake 3.16+, C++17 compiler. SDL2, SDL2_image, SDL2_ttf, SDL2_mixer, Box2D, Lua, and LuaBridge are bundled under `src/thirdParty`.

```bash
cmake -S . -B cmakeBuild
cmake --build cmakeBuild
./cmakeBuild/GameEngine
```

The binary looks for a `resources/` directory relative to the working directory.  
Swap `resources/` for `hw7demo/` or `hw8demo/` to run the included demo projects.

---

## Demo Projects

| Folder       | Description                                                                                                                      |
| ------------ | -------------------------------------------------------------------------------------------------------------------------------- |
| `resources/` | Particle showcase — layered candle flame built from four stacked `ParticleSystem` components                                     |
| `hw7demo/`   | Top-down stealth game — cone-of-vision AI, dialogue system, turn-based enemy movement, multi-level scene transitions, fog of war |
| `hw8demo/`   | Physics sandbox — Box2D rigidbodies, bounciness, kinematic platforms, raycasting victory detection                               |

---

## Dependencies

| Library                                               | Purpose                            |
| ----------------------------------------------------- | ---------------------------------- |
| [SDL2](https://www.libsdl.org/)                       | Window, renderer, input events     |
| [SDL2_image](https://wiki.libsdl.org/SDL2_image)      | PNG/JPG texture loading            |
| [SDL2_ttf](https://wiki.libsdl.org/SDL2_ttf)          | TrueType font rendering            |
| [SDL2_mixer](https://wiki.libsdl.org/SDL2_mixer)      | Audio playback                     |
| [Box2D](https://box2d.org/)                           | 2D rigid body physics              |
| [Lua 5.4](https://www.lua.org/)                       | Scripting VM                       |
| [LuaBridge](https://github.com/vinniefalco/LuaBridge) | C++/Lua binding layer              |
| [GLM](https://github.com/g-truc/glm)                  | Vector and matrix math             |
| [RapidJSON](https://rapidjson.org/)                   | JSON parsing for scenes and config |

---

## Previous Development History

<img width="1307" height="349" alt="Screenshot 2025-06-07 at 17 20 03" src="https://github.com/user-attachments/assets/b83653d4-2c04-45bb-9f7e-4baea216ac5e" />

---

## Author

**Hyungjun Kim** — University of Michigan

---

## License

MIT License — free to use, modify, and distribute.
