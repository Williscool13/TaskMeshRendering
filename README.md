# Task and Mesh Shaders: A Practical Guide

Practical implementation of task and mesh shaders in Vulkan using Slang. Demonstrates basic mesh shader pipelines, GPU-driven culling, and progressive pipeline improvements from simple examples to more advanced techniques.

Article: https://www.williscool.com/technical/task-mesh-rendering/task-mesh-rendering.md.html

## What's Inside

- Basic Task/Mesh Pipeline - Simple Sascha Willems-style sample showing fundamental concepts
- Mesh-Only Rendering - Mesh shaders without task shaders for baseline comparison
- Task + Mesh with Culling - Full pipeline with frustum and backface culling at meshlet granularity

Test scene renders the Stanford Bunny (69,451 vertices, 137,384 triangles) subdivided into meshlets using meshoptimizer.

## Build & Run
Everything you need is included - assets, dependencies, and code are all packaged in this repo.

**Requirements:**

- CMake 3.20+
- Visual Studio 2022
- Vulkan SDK 1.4+ (includes slangc compiler)
- Slang 2025.17.2 or newer
- GPU with mesh shader support (NVIDIA Turing+, AMD RDNA2+, Intel Arc)

**Build Instructions:**

1. Open CMake GUI
2. Set source directory to the cloned repo
3. Set build directory to repo/build
4. Click "Configure" → Select "Visual Studio 17 2022"
5. Click "Generate"
6. Click "Open Project" or navigate to build/ and open the .sln file
7. Build in Visual Studio (Release recommended). Right-click taskMeshRendering project and select "Build"
8. Copy the assets folder from the repo root into the executable folder
9. Copy the compiled shader folder (in the build directory) into the executable folder

## Controls

**Camera:**
- WASD - Move camera
- Left CTRL/Space - Move down/up
- Mouse - Look around
- O/P - Decrease/Increase camera speed
- F - Focus application
- Escape - Quit

**Render Mode:**
The project demonstrates three rendering modes accessible through the imgui interface:
1. Basic (Sascha)
2. Mesh Only
3. Task + Mesh (Culling)

Culling statistics are displayed in real-time showing frustum culled, backface culled, and total rendered meshlets.
