#pragma once

#include <memory>

#include "RHICommand.hpp"

class RHIDevice : public RHIResource {
public:
	virtual std::unique_ptr<RHIBuffer>         createBuffer(const RHIBufferDesc& desc) = 0;
	virtual std::unique_ptr<RHITexture>        createTexture(const RHITextureDesc& desc) = 0;
	virtual std::unique_ptr<RHISampler>        createSampler(const RHISamplerDesc& desc) = 0;
	virtual std::unique_ptr<RHIShader>         createShader(const RHIShaderDesc& desc) = 0;
	virtual std::unique_ptr<RHIStagingTexture> createStagingTexture(const RHITextureDesc& desc) = 0;
	virtual std::unique_ptr<RHIFrameBuffer>    createFrameBuffer(const RHIFrameBufferDesc& desc) = 0;

	virtual std::unique_ptr<RHIInputLayout>      createInputLayout(const RHIInputLayoutDesc& desc) = 0;
	virtual std::unique_ptr<RHIDescriptorLayout> createDescriptorLayout(const RHIDescriptorLayoutDesc& desc) = 0;
	virtual std::unique_ptr<RHIDescriptorSet>    createDescriptorSet(const RHIDescriptorSetDesc& desc, const RHIDescriptorLayout& layout) = 0;
	virtual std::unique_ptr<RHICommandList>      createCommandList(const RHICommandListDesc& desc) = 0;
	virtual std::unique_ptr<RHIGraphicsPipeline> createGraphicsPipeline(const RHIGraphicsPipelineDesc& desc, const RHIFrameBuffer& frameBuffer) = 0;

	virtual void* mapBuffer(RHIBuffer* buffer, RHIAccessMode mode) const = 0;
	virtual void  unmapBuffer(RHIBuffer* buffer) const = 0;
	virtual void* mapStagingTexture(RHIStagingTexture* staging_texture, RHIAccessMode mode) const = 0;
	virtual void  unmapStagingTexture(RHIStagingTexture* staging_texture) const = 0;
	virtual void  bindBufferMemory(RHIBuffer* buffer, uint64_t offset = {}) const = 0;
	virtual void  bindTextureMemory(RHITexture* texture, uint64_t offset = {}) const = 0;

	virtual void writeDescriptorSet(RHIDescriptorSet* set, const RHIDescriptorSetDesc& desc) = 0;
	virtual void executeCommandList(RHICommandList* command_list) = 0;

	virtual void waitIdle() = 0;
};
