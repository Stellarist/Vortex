#include "GpuMesh.hpp"

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

	vertex_count = vertices.size();
	index_count = indices.size();

	if (!vertices.empty()) {
		vertex_buffer = Buffer::createStatic(
		    *this->context,
		    vk::BufferUsageFlagBits::eVertexBuffer,
		    vertices.data(),
		    vertices.size() * sizeof(Vertex));
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
