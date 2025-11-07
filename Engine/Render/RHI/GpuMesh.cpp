#include "GpuMesh.hpp"

#include <numeric>

#include "Render/Graphics/Context.hpp"
#include "Scene/Resources/SubMesh.hpp"

GpuMesh::GpuMesh(Context& context,
    const SubMesh&        submesh,
    DescriptorSetLayout&  layout,
    DescriptorPool&       pool) :
    context(&context), submesh(&submesh)
{
	const auto& vertices = submesh.getVertices();
	const auto& indices = submesh.getIndices();
	const auto& attributes = submesh.getAttributes();

	vertex_count = submesh.getVerticesCount();
	index_count = submesh.getIndicesCount();

	if (!vertices.empty() && vertex_count > 0) {
		auto source_stride = std::accumulate(attributes.begin(), attributes.end(), 0u,
		    [](uint32_t sum, const auto& pair) {
			    return sum + pair.second.size;
		    });

		const auto* pos_attribute = submesh.getAttribute("POSITION");
		const auto* normal_attribute = submesh.getAttribute("NORMAL");
		const auto* uv_attribute = submesh.getAttribute("TEXCOORD_0");
		const auto* color_attribute = submesh.getAttribute("COLOR_0");

		std::vector<GpuVertex> gpu_vertices(vertex_count);
		const uint8_t*         src_data = reinterpret_cast<const uint8_t*>(vertices.data());

		for (uint32_t i = 0; i < vertex_count; i++) {
			const uint8_t* vertex_data = src_data + i * source_stride;

			if (pos_attribute)
				std::memcpy(&gpu_vertices[i].pos, vertex_data + pos_attribute->offset, sizeof(glm::vec3));

			if (normal_attribute)
				std::memcpy(&gpu_vertices[i].normal, vertex_data + normal_attribute->offset, sizeof(glm::vec3));

			if (uv_attribute)
				std::memcpy(&gpu_vertices[i].uv, vertex_data + uv_attribute->offset, sizeof(glm::vec2));

			if (color_attribute)
				std::memcpy(&gpu_vertices[i].color, vertex_data + color_attribute->offset, sizeof(glm::vec4));
		}

		vertex_buffer = Buffer::createStatic(
		    *this->context,
		    vk::BufferUsageFlagBits::eVertexBuffer,
		    gpu_vertices.data(),
		    gpu_vertices.size() * sizeof(GpuVertex));
	}

	if (!indices.empty()) {
		index_buffer = Buffer::createStatic(
		    *this->context,
		    vk::BufferUsageFlagBits::eIndexBuffer,
		    indices.data(),
		    indices.size() * sizeof(uint32_t));
	}

	object_uniform = Buffer::createDynamic(
	    *this->context,
	    vk::BufferUsageFlagBits::eUniformBuffer,
	    &object_data,
	    sizeof(GpuObjectData));

	object_descriptor = pool.allocate(layout);
	object_descriptor.update(context.getDevice(), 0, vk::DescriptorType::eUniformBuffer, object_uniform.get());
}

void GpuMesh::draw(vk::CommandBuffer command_buffer)
{
	command_buffer.drawIndexed(index_count, 1, 0, 0, 0);
}

void GpuMesh::bind(vk::CommandBuffer command_buffer, vk::PipelineLayout pipeline_layout)
{
	command_buffer.bindVertexBuffers(0, vertex_buffer->get(), {0});
	command_buffer.bindIndexBuffer(index_buffer->get(), 0, vk::IndexType::eUint32);
	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout,
	    2, object_descriptor.get(), {});
}

void GpuMesh::setModelMatrix(const glm::mat4& model)
{
	object_data.model = model;
}

void GpuMesh::updateUniforms()
{
	object_uniform->upload(&object_data, sizeof(GpuObjectData));
}

DescriptorSet GpuMesh::getDescriptor() const
{
	return object_descriptor;
}

const SubMesh* GpuMesh::getSubMesh() const
{
	return submesh;
}

vk::Buffer GpuMesh::getVertexBuffer() const
{
	return vertex_buffer->get();
}

vk::Buffer GpuMesh::getIndexBuffer() const
{
	return index_buffer->get();
}

uint32_t GpuMesh::getVertexCount() const
{
	return vertex_count;
}

uint32_t GpuMesh::getIndexCount() const
{
	return index_count;
}
