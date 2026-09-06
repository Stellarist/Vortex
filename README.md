# Vortex

Maybe a toy Vulkan Engine in future, but just a toy Demo at now.

## 当前架构

```text
Application / Editor
        |
        +----> World ----> Actor / Component / Asset
        |                     |
        |                     v
        +----> Renderer -> SceneExtractor -> SceneSnapshot
                                      -> RenderScene -> RenderFrame
                                                     -> build RDG
                              |
                              v
                         Passes -> RHI -> Vulkan
```

核心责任：

- `World` 持有逻辑世界，不依赖 Graphics；
- `SceneExtractor` 把 World 转换成与游戏对象解耦的 `SceneSnapshot`；
- `RenderScene` 维护 GPU Scene 和裁剪结果，输出只包含 `RenderView` 与 `DrawList` 的 `RenderFrame`；
- `RenderCore` 只定义 Renderer 与 RenderPass 共享的帧数据和 Shader ABI；
- `Renderer` 是每帧建图的调用方，知道有哪些 Pass；
- `Passes` 是临时 typed pass，只声明资源访问并录制命令；
- `RDG` 根据读写关系编译依赖、裁剪、生命周期和 barrier，然后执行；
- `RHI` 是 backend 无关的资源与命令接口；
- `Vulkan` 实现 RHI，并使用 Vulkan Dynamic Rendering。

## 当前帧流程

```text
main loop
  -> Renderer::tick
     -> SceneExtractor::extract(World)
     -> RenderScene::update(SceneSnapshot)
     -> RenderScene::prepareFrame
     -> RHIContext::beginFrame
     -> Renderer::draw
        -> register external Backbuffer
        -> create transient Shadow / SceneColor / SceneDepth / optional GBuffer
        -> ShadowPass
        -> ForwardPass (opaque)，或 GBufferPass + DeferredPass
        -> ForwardPass (transparent): sorted alpha blend + read-only depth（有透明 draw 时）
	    -> BlitPass -> Backbuffer
        -> RDG::compile + execute: resource allocation + barriers + debug markers
        -> ImGui callback
     -> RHIContext::endFrame
        -> submit + present
```

## 目录

```text
Engine/
  Core/                       通用类型、Math、Geometry、Clock、Input、Event、File、Log
  Runtime/World/              Object、Assets、Components、Actor/World
  Runtime/RHI/                backend 无关接口
  Runtime/Vulkan/             Vulkan backend
  Runtime/RDG/                Render Dependency Graph
  Runtime/RenderCore/         RenderFrame、DrawList 与 ShaderTypes 等共享契约
  Runtime/RenderPass/         基础 Pass 与具体 RenderShaders
  Runtime/Renderer/           World 提取、GPU Scene、资源缓存、配置与每帧建图
  Editor/                     Window、Importer、CameraController、ImGui、Application
  Shaders/                    Slang shader
Tests/                        按模块组织的无窗口测试与 Fake RHI
Docs/                         架构、工作流、测试方案和路线图
Spec/                         按模块维护的接口、实现和算法说明
Scripts/                      shader 编译与资源复制
```

## 构建

渲染层直接使用 RHI：`RenderShaders` 缓存各阶段的 `RHIShader`，`SceneResources` 管理资产对应的 GPU 资源，`RenderScene` 创建共享的 Scene/Material/Object Layout，各 Pass 显式创建自己的 Pipeline 和 Pass 局部 BindingSet。绑定槽位与 Slang 中的 `vk::binding` 对应；反射文件仅用于构建和测试时核对 ABI，运行时不依赖它们。

需要 CMake、MSVC、Vulkan SDK 和 vcpkg，并设置 `VCPKG_ROOT`。

```powershell
cmake --preset msvc
cmake --build --preset msvc-debug
```

运行：

```powershell
.\Build\Debug\Vortex.exe
```
