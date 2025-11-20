# Clustered Deferred Renderer

A modern C++ OpenGL clustered deferred renderer. This project uses deferred rendering to render scenes with many dynamic lights by assigning lights to 3D clusters.

## Features

- Clustered light culling in fragment shader (no compute shaders)
- Deferred rendering with:
  - Geometry pass (G-buffer)
  - Lighting pass using clustered light assignment
- glTF 2.0 model loading (via `cgltf`)
- Custom camera and input controller
- Basic Blinn-Phong lighting
- Optional normal/specular/emissive/occlusion texture support
- ImGui interface for model loading and editing lights

## Screenshots

![car](assets/images/car.gif)

![backpack](assets/images/backpack.gif)

## How It Works

- **G-buffer** stores position, normal, and albedo/specular info per fragment.
- **Cluster division**: 3D frustum is split into X × Y × Z clusters.
- **Light culling**: Each light’s bounding sphere is tested against cluster AABBs in the fragment shader.
- **Lighting**: Each fragment fetches relevant lights for its cluster and computes lighting (Blinn-Phong).


## Controls

| Input         | Action          |
|---------------|-----------------|
| W / A / S / D | Move camera     |
| Space         | Move up         |
| Shift         | Move down       |
| Mouse Move    | Look around     |
| Scroll        | Zoom in/out     |
| ESC           | Recapture Mouse |

## Build Instructions

### Dependencies

- OpenGL 3.3+
- CMake
- GLFW
- GLAD
- GLM

### Build (macOS/Linux/Windows)

```bash
git clone https://github.com/Lucas-Wng/ClusteredDeferredRenderer.git
cd ClusteredDeferredRenderer
mkdir build && cd build
cmake ..
make
./ClusteredDeferredRenderer
```

