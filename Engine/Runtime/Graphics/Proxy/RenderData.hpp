export module Runtime.Graphics:RenderData;

import Core;
import Runtime.World;
import :RHIPipeline;
import :RHIContext;

export namespace Vortex {

inline constexpr uint32 MAX_LIGHTS_COUNT = 32;

class RenderResource {
protected:
	RHIContext* context{};

public:
	RenderResource(RHIContext& context) : context(&context) {}
	virtual ~RenderResource() = default;

	RHIContext& getContext() const { return *context; }
	RHIDevice&  getDevice() const { return context->getDevice(); }
};

struct RenderVertex {
	Vec3 pos{0.0f};
	Vec3 normal{0.0f, 0.0f, 1.0f};
	Vec2 uv{0.0f};
	Vec4 color{1.0f};

	static RHIVertexBindingDesc binding(uint32 binding = 0);

	static std::vector<RHIVertexAttributeDesc> attributes(uint32 binding = 0);
};

struct RenderCameraData {
	Mat4 view{1.0f};
	Mat4 projection{1.0f};
	Vec4 position{0.0f, 0.0f, 0.0f, 1.0f};

	static RenderCameraData convert(const Camera& camera);
};

struct RenderLightData {
	Vec4 position;
	Vec4 direction;
	Vec4 color;
	Vec4 params;

	static RenderLightData convert(const Light& light);
};

struct RenderObjectData {
	Mat4 model{1.0f};

	static RHIBindingLayoutDesc layout(uint32 binding = 0);
};

struct RenderMaterialData {
	Vec4  albedo{1.0f};
	float metallic{0.0f};
	float roughness{1.0f};

	static RHIBindingLayoutDesc layout(uint32 binding = 0);
};

struct RenderSceneData {
	RenderCameraData camera;
	RenderLightData  lights[MAX_LIGHTS_COUNT];
	Vec4             ambient_color{0.1f, 0.1f, 0.1f, 1.0f};
	uint32           light_count{0};

	static RHIBindingLayoutDesc layout(uint32 binding = 0);
};

}        // namespace Vortex
