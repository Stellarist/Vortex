module Runtime.D3D12;

import d3d12;

namespace Vortex {

RHIRef<RHIShader> D3D12Device::createShader(const RHIShaderDesc& desc, std::span<const std::byte> bytecode)
{
	validateRHIShaderDesc(desc, bytecode);
	CHECK(bytecode.size() >= 4 && std::memcmp(bytecode.data(), "DXBC", 4) == 0, "D3D12 shaders require a DXIL container");

	return makeRHIRef<D3D12Shader>(desc, bytecode);
}

}        // namespace Vortex
