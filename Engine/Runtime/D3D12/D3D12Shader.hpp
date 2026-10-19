export module Runtime.D3D12:Shader;

import d3d12;
import Core;
import Runtime.RHI;

export namespace Vortex {

class D3D12Shader : public RHIShader {
private:
	RHIShaderDesc desc{};

	std::vector<std::byte> bytecode{};

public:
	D3D12Shader(RHIShaderDesc desc, std::span<const std::byte> bytecode) :
	    desc(std::move(desc)), bytecode(bytecode.begin(), bytecode.end()) {}

	const RHIShaderDesc& getDesc() const noexcept override { return desc; }

	D3D12_SHADER_BYTECODE getBytecode() const noexcept { return {bytecode.data(), bytecode.size()}; }
};


}        // namespace Vortex
