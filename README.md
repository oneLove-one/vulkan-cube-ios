# Vulkan Cube for iOS (MoltenVK)

Minimal iOS app that renders a spinning 3D cube with the **Vulkan API** on top of
Apple's Metal GPU, using [MoltenVK](https://github.com/KhronosGroup/MoltenVK).
The IPA is built in **GitHub Actions** (macOS runner with Xcode) — no Mac required.

## What is inside

```
Sources/
  main.mm                  UIApplicationMain
  AppDelegate.mm           window + root view controller
  CubeView.mm              UIView backed by CAMetalLayer, CADisplayLink loop
  VulkanRenderer.h/.cpp    Vulkan 1.0 renderer (instance, device, swapchain,
                           depth buffer, pipeline, push-constant MVP, cube)
Shaders/
  cube.vert / cube.frag    GLSL 450, compiled to SPIR-V in CI
project.yml                XcodeGen project description
.github/workflows/ios.yml  CI: toolchain -> MoltenVK -> shaders -> xcodebuild -> IPA
```

The renderer is plain C++ (`VulkanRenderer.cpp`) so it can be edited without touching
Objective-C. Shaders are compiled with `glslangValidator -V` into SPIR-V, which is
what MoltenVK consumes.

## Build (GitHub Actions)

Push to `main` (or run the workflow manually). The artifact **VulkanCube-ipa**
contains `VulkanCube.ipa`.

## Install on iPhone (free Apple ID)

No special entitlements are used, so the unsigned IPA can be signed with a free
Apple ID via Sideloadly / AltStore:

1. Connect the iPhone (Windows PC with Sideloadly, or macOS).
2. Load `VulkanCube.ipa`, enter the Apple ID, Start.
3. On the iPhone trust the developer certificate:
   Settings → General → VPN & Device Management → Apple Development: <id> → Trust.
4. Free accounts: the app stops launching after 7 days; re-sign to refresh.

## Build locally (macOS)

```bash
brew install xcodegen glslang
curl -L -o MoltenVK-ios.tar \
  https://github.com/KhronosGroup/MoltenVK/releases/download/v1.4.2/MoltenVK-ios.tar
tar xf MoltenVK-ios.tar
glslangValidator -V Shaders/cube.vert -o Shaders/cube.vert.spv
glslangValidator -V Shaders/cube.frag -o Shaders/cube.frag.spv
xcodegen generate
open VulkanCube.xcodeproj   # run on a device or simulator (arm64)
```

## Ideas to play with

- `kVertices` in `VulkanRenderer.cpp` — geometry, per-face colors.
- `matRotationX/Y` angles in `draw()` — rotation speed and axes.
- `kCubeSize` — size.
- `cube.frag` — add lighting/gradients (recompile shaders and push).
