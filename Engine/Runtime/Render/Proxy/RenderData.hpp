#pragma once

#include <glm/glm.hpp>

#include "Runtime/Render/RHI/RHIPipeline.hpp"
#include "Runtime/Render/RHI/RHIContext.hpp"
#include "Runtime/World/Components/Camera.hpp"
#include "Runtime/World/Components/Light.hpp"

constexpr uint32_t MAX_LIGHTS_COUNT = 32;

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
	glm::vec3 pos{0.0f};
	glm::vec3 normal{0.0f, 0.0f, 1.0f};
	glm::vec2 uv{0.0f};
	glm::vec4 color{1.0f};

	static RHIVertexBindingDesc binding(uint32_t binding = 0);

	static std::vector<RHIVertexAttributeDesc> attributes(uint32_t binding = 0);
};

struct RenderCameraData {
	glm::mat4 view{1.0f};
	glm::mat4 projection{1.0f};
	glm::vec4 position{0.0f, 0.0f, 0.0f, 1.0f};

	static RenderCameraData convert(const Camera& camera);
};

struct RenderLightData {
	glm::vec4 position;
	glm::vec4 direction;
	glm::vec4 color;
	glm::vec4 params;

	static RenderLightData convert(const Light& light);
};

struct RenderObjectData {
	glm::mat4 model{1.0f};

	static RHIBindingLayoutDesc layout(uint32_t binding = 0);
};

struct RenderMaterialData {
	glm::vec4 albedo{1.0f};
	float     metallic{0.0f};
	float     roughness{1.0f};

	static RHIBindingLayoutDesc layout(uint32_t binding = 0);
};

struct RenderSceneData {
	RenderCameraData camera;
	RenderLightData  lights[MAX_LIGHTS_COUNT];
	glm::vec4        ambient_color{0.1f, 0.1f, 0.1f, 1.0f};
	uint32_t         light_count{0};

	static RHIBindingLayoutDesc layout(uint32_t binding = 0);
};
