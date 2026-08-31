export module Runtime.Graphics:RHI.Validation;

import Core;
import :RHI.Command;

export namespace Vortex {

RHIBufferViewDesc normalizeRHIBufferViewDesc(const RHIBufferViewDesc& desc);
RHITextureViewDesc normalizeRHITextureViewDesc(const RHITextureViewDesc& desc);

void validateRHIBufferDesc(const RHIBufferDesc& desc);
void validateRHITextureDesc(const RHITextureDesc& desc);
void validateRHISamplerDesc(const RHISamplerDesc& desc);
void validateRHIShaderDesc(const RHIShaderDesc& desc, std::span<const std::byte> bytecode);
void validateRHIFramebufferDesc(const RHIFramebufferDesc& desc);
void validateRHIInputLayoutDesc(const RHIInputLayoutDesc& desc);
void validateRHIBindingLayoutDesc(const RHIBindingLayoutDesc& desc);
void validateRHIBindingSetDesc(const RHIBindingSetDesc& desc, const RHIBindingLayoutDesc& layout);
void validateRHIFramebufferInfo(const RHIFramebufferInfo& info);
void validateRHIGraphicsPipelineDesc(const RHIGraphicsPipelineDesc& desc);
void validateRHIComputePipelineDesc(const RHIComputePipelineDesc& desc);

}        // namespace Vortex
