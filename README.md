# Clustered Deferred Renderer

A C++23/OpenGL 3.3 clustered deferred renderer with glTF loading, dynamic lights, and an ImGui debug panel.

This project demonstrates a full deferred pipeline where lights are assigned to view-space frustum clusters on CPU, uploaded to a GPU texture, and consumed in the lighting shader for per-pixel evaluation.

## Demo

![car](assets/images/car.gif)
![backpack](assets/images/backpack.gif)

## Highlights

- Deferred shading pipeline with G-buffer + fullscreen lighting pass
- Clustered light assignment with logarithmic depth slicing
- glTF 2.0 loading via `cgltf` + `stb_image`
- Normal mapping and specular/gloss support
- ImGui runtime tools for model path loading and live light editing
- GoogleTest unit tests for camera math, clustering math, intersection logic, and scene normalization

## Rendering Pipeline

1. Geometry pass (`shaders/geometry.vert`, `shaders/geometry.frag`)
   - Renders meshes into a G-buffer (`gPosition`, `gNormal`, `gAlbedoSpec`).
   - `gPosition`: view-space position, `RGB16F`
   - `gNormal`: view-space normal, `RGB16F`
   - `gAlbedoSpec`: albedo + gloss/spec, `RGBA8`

2. Cluster construction (`src/ClusterMath.h`, used by `src/DeferredRenderer.cpp`)
   - View frustum is partitioned into `16 x 9 x 24` clusters (`3456` total).
   - Z is sliced logarithmically between near/far plane.

3. Light assignment (CPU)
   - Every light sphere is tested against every cluster AABB in view space.
   - Per-cluster light lists are capped at `MAX_LIGHTS_PER_CLUSTER = 100`.
   - Global uploaded light array is capped to 256 lights per frame.
   - Flattened light indices are uploaded to `clusterLightTex` (`GL_R32I`).

4. Lighting pass (`shaders/lighting.frag`)
   - Reconstructs cluster index from screen pixel and view-space depth.
   - Fetches relevant light indices from `clusterLightTex`.
   - Accumulates Blinn-Phong diffuse/specular with attenuation and ambient term.

## Project Layout

- `src/Application.*`: app lifecycle, frame loop, ImGui panel, input handling
- `src/DeferredRenderer.*`: G-buffer setup, render passes, cluster texture upload
- `src/ClusterMath.h`: pure clustering/intersection math used by runtime and tests
- `src/Scene.*`: model ownership, normalization, light animation/state
- `src/SceneMath.h`: pure scene normalization math
- `src/ModelLoader.*`: glTF parsing, mesh flattening, texture loading/caching
- `src/camera.h`, `src/CameraController.*`: camera state and movement logic
- `shaders/`: geometry and lighting shaders
- `tests/`: GoogleTest unit tests

## Dependencies

- C++23 compiler
- OpenGL 3.3+ core profile
- CMake 3.31+ (as currently required by `CMakeLists.txt`)
- GLFW3

Bundled in repo:
- GLAD (`src/glad.c`, `include/glad`)
- GLM (`include/glm`)
- ImGui (`extern/imgui`)
- cgltf (`include/cgltf.h`)
- stb_image (`include/stb_image.h`)

## Build

```bash
git clone https://github.com/Lucas-Wng/ClusteredDeferredRenderer.git
cd ClusteredDeferredRenderer
cmake -S . -B build
cmake --build build -j
```

Run:

```bash
./build/ClusteredDeferredRenderer
```

Notes:
- If `find_package(GTest)` does not find a local install, CMake fetches GoogleTest automatically during configure.
- On first configure, that fallback needs internet access.

## Tests

Build and run unit tests:

```bash
cmake -S . -B build
cmake --build build --target CameraUnitTests -j
ctest --test-dir build --output-on-failure
```

Current test coverage includes:
- Camera defaults, movement, clamping, and view matrix behavior
- Sphere/AABB intersection correctness
- Frustum slicing and cluster AABB generation
- Cluster light assignment capacity and limit handling
- Scene normalization matrix behavior (including degenerate bounds)

## Controls

| Input | Action |
|---|---|
| `W / A / S / D` | Move camera |
| `Space` | Move up |
| `Left Shift` | Move down |
| `Mouse Move` | Look around |
| `Scroll` | Zoom |
| `ESC` | Toggle cursor capture |

## ImGui Debug Panel

At runtime you can:
- Inspect FPS and GPU frame time
- Load a glTF by path
- Add dynamic lights (position, radius, color, intensity)
- Toggle procedural light animation
- Inspect total light count

Default model path:
- `assets/models/backpack/scene.gltf`

Alternative sample model:
- `assets/models/porche/scene.gltf`

## Technical Notes

- G-buffer stores view-space position/normal to simplify lighting calculations.
- Base color textures are uploaded as sRGB (`GL_SRGB8_ALPHA8`) when detected by filename.
- Cluster selection in shader uses view-space depth (`-fragPosVS.z`) and logarithmic mapping.
- Cluster light list texture dimensions:
  - Width: `MAX_LIGHTS_PER_CLUSTER`
  - Height: `CLUSTER_X * CLUSTER_Y * CLUSTER_Z`

## Known Limitations

- Light assignment is CPU-side `O(lights * clusters)` and can become a bottleneck at very high light counts.
- Shading model is Blinn-Phong (not full PBR).
- No shadows yet.
- Material handling is partial and focused on core maps used by this renderer.

## Roadmap Ideas

- Move cluster assignment to compute shader
- Add shadow mapping
- Add PBR BRDF pipeline
- Add debug visualizations for cluster occupancy and light density
- Add performance benchmark presets and documented metrics
