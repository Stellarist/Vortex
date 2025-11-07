#pragma once

#include <vulkan/vulkan.hpp>

#include "GpuData.hpp"
#include "Render/Graphics/Buffer.hpp"
#include "Render/Graphics/Descriptor.hpp"
#include "Scene/Resources/SubMesh.hpp"

class GpuMesh {
private:
	std::unique_ptr<Buffer> vertex_buffer;
	std::unique_ptr<Buffer> index_buffer;
	uint32_t                vertex_count{};
	uint32_t                index_count{};

	DescriptorSet object_descriptor;

	std::unique_ptr<Buffer> object_uniform;
	GpuObjectData           object_data;

	const SubMesh* submesh{};

	Context* context{};

public:
	GpuMesh(Context&         context,
	    const SubMesh&       submesh,
	    DescriptorSetLayout& layout,
	    DescriptorPool&      pool);
	~GpuMesh() = default;

	GpuMesh(const GpuMesh&) = delete;
	GpuMesh& operator=(const GpuMesh&) = delete;

	GpuMesh(GpuMesh&&) noexcept = default;
	GpuMesh& operator=(GpuMesh&&) noexcept = default;

	void draw(vk::CommandBuffer command_buffer);
	void bind(vk::CommandBuffer command_buffer, vk::PipelineLayout pipeline_layout);

	void setModelMatrix(const glm::mat4& model);
	void updateUniforms();

	DescriptorSet  getDescriptor() const;
	const SubMesh* getSubMesh() const;

	vk::Buffer getVertexBuffer() const;
	vk::Buffer getIndexBuffer() const;

	uint32_t getVertexCount() const;
	uint32_t getIndexCount() const;
};
