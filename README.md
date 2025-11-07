# Vortex

Maybe a toy Vulkan Engine in future, but just a toy Demo at now.

Until Version 2.0:

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                              LAYERS                                         │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 5: APPLICATION LAYER (Orchestration)                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│  Application, Window, Widget                                                │
│  • Entry point & main loop                                                  │
│  • Connects World ←→ Renderer (data flow)                                   │
│  • UI integration                                                           │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 4: HIGH-LEVEL SYSTEMS (Parallel, Independent)                         │
├─────────────────────────────┬─┬─────────────────────────────────────────────┤
│  SCENE SYSTEM               │ │  RENDERING SYSTEM                           │
│  (World/Scene)              │ │  (Renderer)                                 │
│                             │ │                                             │
│  ┌────────────────────────┐ │ │  ┌───────────────────────────────────────┐  │
│  │  World                 │ │ │  │  Renderer                             │  │
│  │  • Active Scene        │ │ │  │  • Frame Management                   │  │
│  │  • Active Camera       │ │ │  │  • Render Loop Control                │  │
│  └────────────────────────┘ │ │  │  • Path Selection (Forward/Deferred)  │  │
│  ┌────────────────────────┐ │ │  └───────────────────────────────────────┘  │
│  │  Scene                 │ │ │  ┌───────────────────────────────────────┐  │
│  │  • Scene Graph (Nodes) │ │ │  │  RenderScene (Adapter/Bridge)         │  │
│  │  • Component Registry  │ │ │  │  • Reads World/Scene                  │  │
│  │  • Resource Registry   │ │ │  │  • Converts to GPU representation     │  │
│  │  • Behaviour System    │ │ │  │  • Manages GPU resources lifecycle    │  │
│  └────────────────────────┘ │ │  └───────────────────────────────────────┘  │
│  ┌────────────────────────┐ │ │  ┌───────────────────────────────────────┐  │
│  │  Node (Scene Graph)    │ │ │  │  Rendering Paths                      │  │
│  │  • Transform           │ │ │  │  • ForwardPath                        │  │
│  │  • Parent/Children     │ │ │  │  • DeferredPath                       │  │
│  │  • Components          │ │ │  │  • Passes (Geometry, Lighting, etc.)  │  │
│  └────────────────────────┘ │ │  └───────────────────────────────────────┘  │
│  ┌────────────────────────┐ │ │                                             │
│  │  Components            │ │ │  Asset Import (Creates Scene Objects)       │
│  │  • Transform           │ │ │  ┌───────────────────────────────────────┐  │
│  │  • Mesh                │ │ │  │  AssetImporter                        │  │
│  │  • Camera              │ │ │  │  • glTF 2.0 Parser                    │  │
│  │  • Light               │ │ │  │  • Creates Scene/Nodes/Components     │  │
│  └────────────────────────┘ │ │  │  • Creates Resources (Materials, etc.)│  │
│  ┌────────────────────────┐ │ │  └───────────────────────────────────────┘  │
│  │  Resources             │ │ │                                             │
│  │  • Material (PBR)      │ │ │                                             │
│  │  • Texture             │ │ │                                             │
│  │  • SubMesh (Geometry)  │ │ │                                             │
│  └────────────────────────┘ │ │                                             │
│  ┌────────────────────────┐ │ │                                             │
│  │  Behaviours            │ │ │                                             │
│  │  • Script Interface    │ │ │                                             │
│  │  • CameraController    │ │ │                                             │
│  └────────────────────────┘ │ │                                             │
└─────────────────────────────┴─┴─────────────────────────────────────────────┘
                                                   │
                                                   │ depends on
                                                   ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 3: RHI LAYER (Render Hardware Interface)                              │
├─────────────────────────────────────────────────────────────────────────────┤
│  GPU Resource Representation (Bridge between Scene and Graphics API)        │
│  ┌────────────────────────────────────────────────────────────────────┐     │
│  │  GpuData (Shader Structures)                                       │     │
│  │  • GpuVertex, GpuObjectData, GpuMaterialData, GpuSceneData         │     │
│  └────────────────────────────────────────────────────────────────────┘     │
│  ┌────────────────────────────────────────────────────────────────────┐     │
│  │  GpuMesh                                                           │     │
│  │  • Converts SubMesh → Vertex/Index Buffers                         │     │
│  │  • Uniform buffer for object transforms                            │     │
│  └────────────────────────────────────────────────────────────────────┘     │
│  ┌────────────────────────────────────────────────────────────────────┐     │
│  │  GpuMaterial                                                       │     │
│  │  • Converts Material → Uniform buffers + Descriptor sets           │     │
│  └────────────────────────────────────────────────────────────────────┘     │
│  ┌────────────────────────────────────────────────────────────────────┐     │
│  │  GpuTexture                                                        │     │
│  │  • Converts Texture → Vulkan Images + Samplers                     │     │
│  └────────────────────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ depends on
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 2: GRAPHICS ABSTRACTION (Vulkan Wrappers)                             │
├─────────────────────────────────────────────────────────────────────────────┤
│  Low-level Vulkan object wrappers (RAII, C++ friendly)                      │
│                                                                             │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐           │
│  │ Context          │  │ Device           │  │ SwapChain        │           │
│  │ • Instance       │  │ • Physical       │  │ • Images         │           │
│  │ • Surface        │  │ • Logical        │  │ • Image Views    │           │
│  │ • Integration    │  │ • Queues         │  │ • Presentation   │           │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘           │
│                                                                             │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐           │
│  │ Buffer           │  │ Image            │  │ Sampler          │           │
│  │ • VBO            │  │ • Textures       │  │ • Filtering      │           │
│  │ • IBO            │  │ • Render Targets │  │ • Address Mode   │           │
│  │ • UBO            │  │ • Layout Trans.  │  └──────────────────┘           │
│  └──────────────────┘  └──────────────────┘                                 │
│                                                                             │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐           │
│  │ CommandPool      │  │ RenderPass       │  │ GraphicsPipeline │           │
│  │ • Command Alloc  │  │ • Attachments    │  │ • Shaders        │           │
│  │ • Command Buffers│  │ • Subpasses      │  │ • Vertex Input   │           │
│  └──────────────────┘  │ • Framebuffers   │  │ • Rasterization  │           │
│                        └──────────────────┘  │ • Depth/Blend    │           │
│  ┌──────────────────┐                        └──────────────────┘           │
│  │ Descriptor       │  ┌──────────────────┐  ┌──────────────────┐           │
│  │ • Layouts        │  │ Sync             │  │ Shader           │           │
│  │ • Pools          │  │ • Fence          │  │ • SPIR-V Loader  │           │
│  │ • Sets           │  │ • Semaphore      │  │ • Reflection     │           │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘           │
│                                                                             │
│  ┌──────────────────┐                                                       │
│  │ GBuffer          │  (Multi-Render-Target for deferred rendering)         │
│  │ • Multiple Images│                                                       │
│  └──────────────────┘                                                       │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ wraps
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 1: CORE SYSTEMS (Foundation, No dependencies on upper layers)         │
├─────────────────────────────────────────────────────────────────────────────┤
│  Utility systems used by all layers                                         │
│                                                                             │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐           │
│  │ Clock            │  │ EventBus         │  │ InputHandler     │           │
│  │ • Delta Time     │  │ • Pub/Sub        │  │ • Keyboard       │           │
│  │ • Total Time     │  │ • Events         │  │ • Mouse          │           │
│  │ • FPS            │  └──────────────────┘  └──────────────────┘           │
│  └──────────────────┘                                                       │
│                                                                             │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐           │
│  │ FileSystem       │  │ PathResolver     │  │ JsonParser       │           │
│  │ • Read/Write     │  │ • Asset Paths    │  │ • Config Loading │           │
│  │ • Binary/Text    │  │ • Config Paths   │  │                  │           │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘           │
│                                                                             │
│  ┌──────────────────┐                                                       │
│  │ Logger           │                                                       │
│  │ • spdlog wrapper │                                                       │
│  └──────────────────┘                                                       │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ uses
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ Layer 0: EXTERNAL DEPENDENCIES                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│  Vulkan SDK 1.4  │  SDL3  │  ImGui  │  glm  │  TinyGLTF  │  spdlog          │
│  nlohmann/json   │  Slang (shader compiler)                                 │
└─────────────────────────────────────────────────────────────────────────────┘

```

```text
┌──────────────────────────────────────────────────────────────────────────┐
│                              MODULES                                     │
└──────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────┐          ┌────────────────────────────────────┐
│     SCENE SYSTEM         │          │        RENDERING SYSTEM            │
│    (Data & Logic)        │          │         (Visualization)            │
├──────────────────────────┤          ├────────────────────────────────────┤
│                          │          │                                    │
│  ┌────────────────────┐  │          │  ┌───────────────────────────────┐ │
│  │  World             │  │  read    │  │  Renderer                     │ │
│  │  • Active Scene    │  │ ──────>  │  │  • Context                    │ │
│  │  • Active Camera   │  │   by     │  │  • Frame Management           │ │
│  └────────────────────┘  │          │  │  • Render Loop                │ │
│           │              │          │  └───────────────────────────────┘ │
│           │              │          │              │                     │
│  ┌────────▼────────────┐ │          │  ┌───────────▼──────────────────┐  │
│  │  Scene              │ │          │  │  RenderScene                 │  │
│  │  • Root Node        │ │          │  │  • Converts Scene to GPU     │  │
│  │  • Component Lists  │ │          │  │  • Descriptor Management     │  │
│  │  • Resource Lists   │ │          │  │  • Draw Call Organization    │  │
│  │  • Behaviour Lists  │ │          │  └──────────────────────────────┘  │
│  └─────────────────────┘ │          │              │                     │
│           │              │          │  ┌───────────▼──────────────────┐  │
│  ┌────────▼────────────┐ │          │  │  Rendering Paths             │  │
│  │  Node (Scene Graph) │ │          │  │  ┌────────────────────────┐  │  │
│  │  • Transform        │ │          │  │  │  ForwardPath           │  │  │
│  │  • Children         │ │          │  │  │  • ForwardPass         │  │  │
│  │  • Components       │ │          │  │  │  • Pipeline            │  │  │
│  │  • Behaviours       │ │          │  │  └────────────────────────┘  │  │
│  └─────────────────────┘ │          │  │  ┌────────────────────────┐  │  │
│           │              │          │  │  │  DeferredPath          │  │  │
│  ┌────────▼────────────┐ │          │  │  │  • GeometryPass        │  │  │
│  │ ┌────────────────┐  │ │          │  │  │  • LightingPass        │  │  │
│  │ │ Components     │  │ │          │  │  │  • GBuffer             │  │  │
│  │ │ • Transform    │  │ │          │  │  │  • Pipelines           │  │  │
│  │ │ • Mesh         │  │ │          │  │  └────────────────────────┘  │  │
│  │ │ • Camera       │  │ │          │  └──────────────────────────────┘  │
│  │ │ • Light        │  │ │          │              │                     │
│  │ └────────────────┘  │ │          │  ┌───────────▼──────────────────┐  │
│  │ ┌────────────────┐  │ │          │  │  GPU Resources (RHI)         │  │
│  │ │ Resources      │  │ │          │  │  • GpuMesh                   │  │
│  │ │• Material      │  │ │          │  │  • GpuMaterial               │  │
│  │ │• Texture       │  │ │          │  │  • GpuTexture                │  │
│  │ │• SubMesh       │  │ │          │  │  • GpuData (structs)         │  │
│  │ └────────────────┘  │ │          │  └──────────────────────────────┘  │
│  │ ┌────────────────┐  │ │          │                                    │
│  │ │ Behaviours     │  │ │          └────────────────────────────────────┘
│  │ │ • Controller   │  │ │                          │
│  │ │• Custom Scripts│  │ │                          │ depends on
│  │ └────────────────┘  │ │                          ▼
│  └─────────────────────┘ │          ┌─────────────────────────────────────┐
│                          │          │   GRAPHICS ABSTRACTION (Low-level)  │
└──────────────────────────┘          ├─────────────────────────────────────┤
                                      │  Vulkan Wrappers:                   │
┌──────────────────────────┐          │  ┌────────────────────────────────┐ │
│   CORE SYSTEMS           │          │  │  Context (Instance, Surface)   │ │
│  (Foundation)            │          │  └────────────────────────────────┘ │
├──────────────────────────┤          │  ┌────────────────────────────────┐ │
│  • Clock (Time)          │          │  │  Device (GPU)                  │ │
│  • EventBus (Events)     │          │  └────────────────────────────────┘ │
│  • InputHandler (Input)  │          │  ┌────────────────────────────────┐ │
│  • Logger (spdlog)       │          │  │  SwapChain (Presentation)      │ │
│  • FileSystem (I/O)      │          │  └────────────────────────────────┘ │
│  • PathResolver          │          │  ┌────────────────────────────────┐ │
│  • JsonParser            │          │  │  CommandPool & CommandBuffer   │ │
└──────────────────────────┘          │  └────────────────────────────────┘ │
            │                         │  ┌────────────────────────────────┐ │
            │ used by all             │  │  Buffer (VBO, UBO, etc.)       │ │
            └─────────────────────────┼─>│  Image (Textures)              │ │
                                      │  │  Sampler                       │ │
┌──────────────────────────┐          │  └────────────────────────────────┘ │
│   ASSET SYSTEM           │          │  ┌────────────────────────────────┐ │
│  (Import & Load)         │          │  │  RenderPass & Framebuffer      │ │
├──────────────────────────┤          │  └────────────────────────────────┘ │
│  AssetImporter           │          │  ┌────────────────────────────────┐ │
│  • glTF 2.0 Loader       │ creates  │  │  GraphicsPipeline              │ │
│  • Scene Parser          │ ───────> │  │  Shader (SPIR-V)               │ │
│  • Material/Texture      │  Scene   │  └────────────────────────────────┘ │
│  • Mesh Parser           │  Objects │  ┌────────────────────────────────┐ │
│  • Default Creation      │          │  │  DescriptorPool, Sets, Layouts │ │
└──────────────────────────┘          │  └────────────────────────────────┘ │
                                      │  ┌────────────────────────────────┐ │
                                      │  │  Sync (Fence, Semaphore)       │ │
                                      │  └────────────────────────────────┘ │
                                      │  ┌────────────────────────────────┐ │
                                      │  │  GBuffer (MRT)                 │ │
                                      │  └────────────────────────────────┘ │
                                      └─────────────────────────────────────┘
                                                      │
                                                      │ wraps
                                                      ▼
                                      ┌──────────────────────────────────────┐
                                      │         VULKAN API 1.4               │
                                      └──────────────────────────────────────┘
```

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                                 FLOWS                                       │
└─────────────────────────────────────────────────────────────────────────────┘

 Application::run()
      │
      ├───────────────────────────────────────────────────────────┐
      │                                                           │
      ▼                                                           ▼
  tickLogic(dt)                                             tickRender(dt)
      │                                                           │
      ▼                                                           ▼
  World::tick(dt)                                         Renderer::tick(dt)
      │                                                           │
      ├─> Scene::update(dt) ────────────────────────────────────> │
      │    └─> Behaviour::update()                                │
      │         • Game logic                                      │
      │         • Transform updates                               │
      │         • Component interactions                          │
      │                                                           │
      │                                                           ▼
      │                                              RenderScene::update(dt)
      │                                                           │
      │                                                           ├─> Read World
      │                                                           ├─> Read Scene
      │                                                           ├─> updateCamera()
      │                                                           ├─> updateLights()
      │                                                           └─> updateMeshes()
      │                                                           │
      │◄──────────────────────────────────────────────────────────┘
      │                                                           │ 
      │                                                           │ 
      ▼                                                           │ 
  (World state updated)                                           │
                                                                  ▼
                                                          Renderer::begin()
                                                                  │
                                                                  ├─> acquireImage()
                                                                  └─> beginCommand()
                                                                  │
                                                                  ▼
                                                          Renderer::draw()
                                                                  │
                                                                  ├─> [Forward Path]
                                                                  │    └─> Draw meshes
                                                                  │
                                                                  └─> [Deferred Path]
                                                                  │    ├─> Geometry Pass
                                                                  │    └─> Lighting Pass
                                                                  │
                                                                  ▼
                                                          Renderer::end()
                                                                  │
                                                                  ├─> endCommand()
                                                                  ├─> submit()
                                                                  └─> present()

```
