export module Runtime.RHI:Shader;

import Core;
import :Resource;
import :Types;

export namespace Vortex {

struct RHIShaderDesc {
	RHIShaderType type{RHIShaderType::None};

	std::string shader_name{};
	std::string entry_point{"main"};

	RHIShaderDesc& setType(RHIShaderType new_type) noexcept
	{
		type = new_type;
		return *this;
	}

	RHIShaderDesc& setShaderName(std::string new_shader_name)
	{
		shader_name = std::move(new_shader_name);
		return *this;
	}

	RHIShaderDesc& setEntryPoint(std::string new_entry_point)
	{
		entry_point = std::move(new_entry_point);
		return *this;
	}
};

class RHIShader : public RHIResource {
public:
	virtual const RHIShaderDesc& getDesc() const noexcept = 0;
};

}        // namespace Vortex
