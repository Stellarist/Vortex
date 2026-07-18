# Vortex

Maybe a toy Vulkan Engine in future, but just a toy Demo at now.

Until Version 3.0:

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                                  LAYERS                                     │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 6: APPLICATION / EDITOR                                               │
├─────────────────────────────────────────────────────────────────────────────┤
│  Application                     Window                    Widget            │
│  • Main loop                     • SDL3 window             • ImGui frame     │
│  • Logic/render orchestration    • Input/events            • Scene UI       │
│  • World/Renderer lifetime       • Vulkan surface          • Render callback│
└─────────────────────────────────────────────────────────────────────────────┘
                    │                    │                    │
                    └────────────────────┼────────────────────┘
                                         ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 5: WORLD / RENDERER                                                   │
├─────────────────────────────────────┬───────────────────────────────────────┤
│ WORLD SYSTEM                        │ RENDERER                              │
│                                     │                                       │
│ ┌─────────────────────────────────┐ │ ┌───────────────────────────────────┐ │
│ │ World                           │ │ │ Renderer                          │ │
│ │ • Active Scene / Camera         │ │ │ • Frame begin/end                 │ │
│ └─────────────────────────────────┘ │ │ • Backbuffer / Depth              │ │
│ ┌─────────────────────────────────┐ │ │ • Graphics pipeline/state         │ │
│ │ Scene                           │ │ │ • Forward/Deferred shader select  │ │
│ │ • Node hierarchy                │ │ │ • UI render callbacks             │ │
│ │ • Component registry            │ │ └───────────────────────────────────┘ │
│ │ • Resource registry             │ │                                       │
│ │ • Behaviour update              │ │ ┌───────────────────────────────────┐ │
│ └─────────────────────────────────┘ │ │ AssetImporter                     │ │
│ ┌───────────────┬─────────────────┐ │ │ • tinygltf loader                 │ │
│ │ Components    │ Resources       │ │ │ • Scene/Node conversion           │ │
│ │ • Transform   │ • SubMesh       │ │ │ • Mesh/Material/Texture import    │ │
│ │ • Mesh        │ • Material      │ │ │ • Camera/Light import             │ │
│ │ • Camera      │ • Texture       │ │ └───────────────────────────────────┘ │
│ │ • Light       │                 │ │                                       │
│ ├───────────────┴─────────────────┤ │                                       │
│ │ Entity / Node / Behaviour       │ │                                       │
│ │ CameraController / AABB / Ray   │ │                                       │
│ └─────────────────────────────────┘ │                                       │
└─────────────────────────────────────┴───────────────────────────────────────┘
                    │                                   │
                    └──────────────┬────────────────────┘
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 4: RENDER PROXY                                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│ ┌─────────────────────────────────────────────────────────────────────────┐ │
│ │ RenderScene                                                             │ │
│ │ • Reads World/Scene              • Scene/Material/Object descriptor sets │ │
│ │ • Camera/Light/Mesh update       • Resource rebuild and material sorting │ │
│ │ • RenderSceneData upload         • Draw traversal                       │ │
│ └─────────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
│ ┌──────────────────────┬──────────────────────┬───────────────────────────┐ │
│ │ RenderMesh           │ RenderMaterial       │ RenderTexture             │ │
│ │ • Vertex/Index       │ • Material uniform   │ • RHITexture              │ │
│ │ • Object uniform     │ • Descriptor set     │ • RHISampler              │ │
│ │ • Draw state         │ • Texture bindings   │ • Source Texture          │ │
│ └──────────────────────┴──────────────────────┴───────────────────────────┘ │
│                                                                             │
│ RenderVertex / RenderCameraData / RenderLightData / RenderObjectData        │
│ RenderMaterialData / RenderSceneData                                        │
└─────────────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 3: RHI (RENDER HARDWARE INTERFACE)                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│ ┌──────────────────────┬──────────────────────┬───────────────────────────┐ │
│ │ Context / Device     │ Resources            │ Descriptors               │ │
│ │ • Frame lifecycle    │ • Buffer             │ • DescriptorLayout        │ │
│ │ • Device factory     │ • Texture 1D-3D/Cube  │ • DescriptorSet           │ │
│ │ • Backbuffer         │ • StagingTexture     │ • Texture SRV/UAV         │ │
│ │ • Command execution  │ • Sampler / Shader   │ • Uniform/Storage Buffer  │ │
│ │                      │ • FrameBuffer         │ • Sampler                 │ │
│ └──────────────────────┴──────────────────────┴───────────────────────────┘ │
│ ┌──────────────────────────────────┬──────────────────────────────────────┐ │
│ │ Pipeline                         │ Command                              │ │
│ │ • InputLayout                    │ • GraphicsState                      │ │
│ │ • Raster/Depth/Blend State       │ • Draw/DrawIndexed                   │ │
│ │ • GraphicsPipeline               │ • Copy/Clear/Write                   │ │
│ │ • Shader/Descriptor Layouts      │ • Buffer/Texture transitions         │ │
│ └──────────────────────────────────┴──────────────────────────────────────┘ │
│                                                                             │
│ RHITypes: Format / ResourceState / Usage / TextureDimension / Subresource   │
└─────────────────────────────────────────────────────────────────────────────┘
                                   │ implemented by
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 2: VULKAN BACKEND                                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│ ┌──────────────────────┬──────────────────────┬───────────────────────────┐ │
│ │ VulkanContext        │ VulkanDevice         │ VulkanQueue               │ │
│ │ • Instance/Surface   │ • RHI object factory │ • Graphics submission     │ │
│ │ • Swapchain/Frames   │ • Memory mapping     │ • Timeline semaphore      │ │
│ │ • Acquire/Present    │ • Descriptor writes  │ • Pending submissions     │ │
│ └──────────────────────┴──────────────────────┴───────────────────────────┘ │
│ ┌──────────────────────┬──────────────────────┬───────────────────────────┐ │
│ │ VulkanCommand        │ VulkanResources      │ VulkanPipeline/Descriptor │ │
│ │ • CommandPool/Buffer │ • VMA allocation     │ • Graphics pipeline       │ │
│ │ • Dynamic rendering  │ • Buffer/Texture     │ • Input layout            │ │
│ │ • State transitions  │ • Sampler/Shader     │ • Descriptor layout/set   │ │
│ │ • Copy/Draw          │ • FrameBuffer        │ • Pipeline layout         │ │
│ └──────────────────────┴──────────────────────┴───────────────────────────┘ │
│                                                                             │
│ vk-bootstrap / Vulkan-Hpp / Vulkan Memory Allocator                         │
└─────────────────────────────────────────────────────────────────────────────┘
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 1: CORE / PLATFORM / THIRD PARTY                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│ Core: Clock / EventBus / Input / FileSystem / PathResolver / Json / Logger  │
│ Platform: Vulkan 1.4 / SDL3                                                 │
│ Libraries: glm / ImGui / spdlog / nlohmann-json / tinygltf / Slang          │
└─────────────────────────────────────────────────────────────────────────────┘
```

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                               DEPENDENCIES                                  │
└─────────────────────────────────────────────────────────────────────────────┘

                           ┌──────────────────┐
                           │   Application    │
                           └───┬─────┬─────┬──┘
                               │     │     │
                ┌──────────────┘     │     └──────────────┐
                ▼                    ▼                    ▼
        ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
        │ Window/Widget│     │    World     │     │   Renderer   │
        └──────┬───────┘     └──────┬───────┘     └──────┬───────┘
               │                    │                    │ owns
               │                    ▼                    ▼
               │             ┌──────────────┐     ┌──────────────┐
               │             │ Scene/Node   │────>│ RenderScene  │
               │             │ Components   │     └──────┬───────┘
               │             │ Resources    │            │ owns
               │             └──────▲───────┘            ▼
               │                    │             ┌────────────────────┐
               │             ┌──────┴───────┐     │ RenderMesh        │
               │             │AssetImporter │     │ RenderMaterial    │
               │             └──────────────┘     │ RenderTexture     │
               │                                  └─────────┬──────────┘
               │                                            │ uses
               │                                            ▼
               │                                  ┌────────────────────┐
               └─────────────────────────────────>│        RHI         │
                                                  └─────────▲──────────┘
                                                            │ implements
                                                  ┌─────────┴──────────┐
                                                  │  Vulkan Backend    │
                                                  └─────────┬──────────┘
                                                            ▼
                                                  ┌────────────────────┐
                                                  │ Vulkan API / GPU   │
                                                  └────────────────────┘
```

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                             CURRENT FRAME FLOW                              │
└─────────────────────────────────────────────────────────────────────────────┘

Application::run()                         [Single Thread]
      │
      ├─> Clock::tick()
      ├─> tickGui(dt)
      │     ├─> Window::pollEvent()
      │     └─> Widget::newFrame()
      │
      ├─> tickLogic(dt)
      │     └─> World::tick(dt)
      │           └─> Scene::update(dt)
      │                 └─> Behaviour::update(dt)
      │
      └─> tickRender(dt)
            └─> Renderer::tick(dt)
                  ├─> RenderScene::update(dt)
                  │     ├─> rebuild resources (when counts change)
                  │     ├─> update camera/lights/transforms
                  │     └─> upload scene/object uniforms
                  │
                  ├─> RHIContext::beginFrame()
                  │     └─> acquire swapchain image / open command list
                  │
                  ├─> clear backbuffer + depth
                  ├─> Renderer::drawScene()
                  │     ├─> create framebuffer/pipeline when needed
                  │     └─> RenderScene::draw()
                  │           └─> Scene Set -> Material Set -> Object Set
                  │                 └─> setGraphicsState() -> drawIndexed()
                  │
                  ├─> Widget render callback
                  └─> RHIContext::endFrame()
                        └─> close -> submit -> present
```
