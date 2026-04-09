# A miniature game engine created by Hyungjun Kim

A simple, Unity-inspired, Lua-based 2D game engine with Box2D Physics.  
Developed for educational purposes at the **University of Michigan, Ann Arbor** as part of **EECS 498: Game Engine Architecture**.

---

![agame_engine_k4E7uA6kG6](https://github.com/user-attachments/assets/d59aa408-033f-4750-923e-d7af300a1507)

---

## 🚀 Overview
This is a simple 2D game engine designed to help understand the principles of game engine architecture while still being powerful enough to build meaningful projects. It features a Lua scripting layer on top of a performant C++ core, which allows experimentation with both high-level design and low-level performance.

---

## ✨ Features

- **Component-Based Lua Architecture**  
  Write gameplay logic in Lua using lifecycle functions (`OnStart`, `OnUpdate`, `OnDestroy`, etc.) and attach them to Actors (gameobjects).

- **Performant C++ Core**  
  The scripting layer sits on top of a high-speed C++ engine that integrates:  
  - [Box2D](https://box2d.org/) physics  
  - Particle systems  
  - Core rendering and input systems  

- **Educational Mission**  
  - MIT licensed — use it freely, even commercially.
  
---

## 🛠️ Technologies Used
- **C++** – Core engine implementation (performance, physics, rendering, input)  
- **Lua** – High-level scripting layer with component lifecycle functions  
- **[SDL2](https://www.libsdl.org/)** – Rendering, window management, and input handling  
- **[Box2D](https://box2d.org/)** – 2D physics simulation (collisions, rigidbodies, forces)
- **[CMake](https://cmake.org/)** – Build system to aid cross-platform development workflow  
- **[GLM](https://github.com/g-truc/glm)** – Mathematics library (vectors, matrices, transformations)  
- **[RapidJSON](https://rapidjson.org/)** – High-performance JSON parsing for engine configuration and data  
- **Custom Particle System** – Built-in fast particle rendering  (Coming Soon!)
- **Engine Framework** – Unity-inspired component-actor system designed with familiarity in mind

---

## 👥 Authors
- **Hyungjun Kim** – Designer & Implementer  

---

## Previous development history
<img width="1307" height="349" alt="Screenshot 2025-06-07 at 17 20 03" src="https://github.com/user-attachments/assets/b83653d4-2c04-45bb-9f7e-4baea216ac5e" />


## 📜 License
This project is licensed under the [MIT License](LICENSE).  
