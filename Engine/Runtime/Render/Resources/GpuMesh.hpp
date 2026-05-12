#pragma once

#include <vulkan/vulkan.hpp>

#include "GpuData.hpp"
#include "Runtime/Render/Backend/VulkanBuffer.hpp"
#include "Runtime/Render/Backend/VulkanDescriptor.hpp"
#include "Runtime/World/Resources/SubMesh.hpp"

class GpuMesh {
private:
	std::unique_ptr<VulkanBuffer> vertex_buffer;
	std::unique_ptr<VulkanBuffer> index_buffer;
	uint32_t                      vertex_count{};
	uint32_t                      index_count{};

	VulkanDescriptorSet object_descriptor;

	std::unique_ptr<VulkanBuffer> object_uniform;
	GpuObjectData                 object_data;

	const SubMesh* submesh{};

	VulkanContext* context{};

public:
	GpuMesh(VulkanContext&         context,
	    const SubMesh&       submesh,
	    VulkanDescriptorSetLayout& layout,
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

	VulkanDescriptorSet  getDescriptor() const;
	const SubMesh* getSubMesh() const;

	vk::Buffer getVertexBuffer() const;
	vk::Buffer getIndexBuffer() const;

	uint32_t getVertexCount() const;
	uint32_t getIndexCount() const;
};
