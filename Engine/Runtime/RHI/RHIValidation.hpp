export module Runtime.RHI:Validation;

import Core;
import :Command;

export namespace Vortex {

RHIBufferViewDesc normalizeRHIBufferViewDesc(const RHIBufferViewDesc& desc);
RHITextureViewDesc normalizeRHITextureViewDesc(const RHITextureViewDesc& desc);
RHITextureSlice normalizeRHITextureSlice(const RHITextureDesc& desc, RHITextureSlice slice);
RHIViewportState normalizeRHIViewportState(const RHIViewportState& state, const RHIFramebufferDesc& framebuffer);

void validateRHITextureCopy(const RHITextureDesc& dst, const RHITextureSlice& dst_slice, const RHITextureDesc& src, const RHITextureSlice& src_slice);
void validateRHIBufferCopy(const RHIBuffer* dst, uint64 dst_offset, const RHIBuffer* src, uint64 src_offset, uint64 size);
void validateRHITextureState(const RHITexture& texture, RHIResourceState expected, std::string_view operation);
void validateRHIBufferState(const RHIBuffer& buffer, RHIResourceState expected, std::string_view operation);
void validateRHIBufferTransition(const RHIBuffer& buffer, RHIResourceState before, RHIResourceState after);
void validateRHIBindingSets(const std::vector<RHIRef<RHIBindingSet>>& sets, const std::vector<RHIRef<RHIBindingLayout>>& layouts);
void validateRHICommandListDesc(const RHICommandListDesc& desc);

void validateRHIBufferDesc(const RHIBufferDesc& desc);
void validateRHITextureDesc(const RHITextureDesc& desc);
void validateRHISamplerDesc(const RHISamplerDesc& desc);
void validateRHIShaderDesc(const RHIShaderDesc& desc, std::span<const std::byte> bytecode);
void validateRHIFramebufferDesc(const RHIFramebufferDesc& desc);
void validateRHIInputLayoutDesc(const RHIInputLayoutDesc& desc);
void validateRHIBindingLayoutDesc(const RHIBindingLayoutDesc& desc);
void validateRHIBindlessLayoutDesc(const RHIBindlessLayoutDesc& desc);
void validateRHIBindingSetDesc(const RHIBindingSetDesc& desc, const RHIBindingLayout& layout);
void validateRHIBindingSetItem(const RHIBindingSetItem& item, const RHIBindlessLayoutDesc& layout);
void validateRHIFramebufferInfo(const RHIFramebufferInfo& info);
void validateRHIGraphicsPipelineDesc(const RHIGraphicsPipelineDesc& desc);
void validateRHIComputePipelineDesc(const RHIComputePipelineDesc& desc);

}        // namespace Vortex
