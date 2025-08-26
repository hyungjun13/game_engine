# A miniature game engine created by hyungjun
<img width="1307" height="349" alt="Screenshot 2025-06-07 at 17 20 03" src="https://github.com/user-attachments/assets/b83653d4-2c04-45bb-9f7e-4baea216ac5e" />

A simple, Unity-inspired, Lua-based 2D game engine with Box2D Physics.  
Developed for educational purposes at the **University of Michigan, Ann Arbor** as part of **EECS 498: Game Engine Architecture**.

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

## 👥 Authors
- **Hyungjun Kim** – Designer & Implementer  

---


## 📜 License
This project is licensed under the [MIT License](LICENSE).  
