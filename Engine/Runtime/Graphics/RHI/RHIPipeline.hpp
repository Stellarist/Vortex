export module Runtime.Graphics:RHI.Pipeline;

import Core;
import :RHI.Binding;
import :RHI.Framebuffer;
import :RHI.Shader;

export namespace Vortex {

struct RHIVertexBindingDesc {
	uint32 binding{};
	uint32 stride{};
	bool   instance{};

	RHIVertexBindingDesc& setBinding(uint32 new_binding) noexcept
	{
		binding = new_binding;
		return *this;
	}

	RHIVertexBindingDesc& setStride(uint32 new_stride) noexcept
	{
		stride = new_stride;
		return *this;
	}

	RHIVertexBindingDesc& setInstance(bool new_instance) noexcept
	{
		instance = new_instance;
		return *this;
	}
};

struct RHIVertexAttributeDesc {
	std::string name{};
	RHIFormat   format{};
	uint32      location{};
	uint32      binding{};
	uint32      offset{};

	RHIVertexAttributeDesc& setName(const std::string& new_name)
	{
		name = new_name;
		return *this;
	}

	RHIVertexAttributeDesc& setFormat(RHIFormat new_format) noexcept
	{
		format = new_format;
		return *this;
	}

	RHIVertexAttributeDesc& setLocation(uint32 new_location) noexcept
	{
		location = new_location;
		return *this;
	}

	RHIVertexAttributeDesc& setBinding(uint32 new_binding) noexcept
	{
		binding = new_binding;
		return *this;
	}

	RHIVertexAttributeDesc& setOffset(uint32 new_offset) noexcept
	{
		offset = new_offset;
		return *this;
	}
};

struct RHIInputLayoutDesc {
	std::vector<RHIVertexBindingDesc>   binding_descs{};
	std::vector<RHIVertexAttributeDesc> attribute_descs{};

	RHIInputLayoutDesc& addBindingDesc(const RHIVertexBindingDesc& new_binding_desc)
	{
		binding_descs.push_back(new_binding_desc);
		return *this;
	}

	RHIInputLayoutDesc& setBindingDescs(const std::vector<RHIVertexBindingDesc>& new_binding_descs)
	{
		binding_descs = new_binding_descs;
		return *this;
	}

	RHIInputLayoutDesc& setAttributeDescs(const std::vector<RHIVertexAttributeDesc>& new_attribute_descs)
	{
		attribute_descs = new_attribute_descs;
		return *this;
	}

	RHIInputLayoutDesc& addAttributeDesc(const RHIVertexAttributeDesc& new_attribute_desc)
	{
		attribute_descs.push_back(new_attribute_desc);
		return *this;
	}
};

class RHIInputLayout : public RHIResource {
public:
	virtual uint32 getAttributeCount() const noexcept = 0;
	virtual const RHIVertexAttributeDesc& getAttributeDesc(uint32 index) const = 0;
};


struct RHIViewportState {
	std::vector<RHIViewport> viewports{};
	std::vector<RHIRect>     scissors{};

	RHIViewportState& addViewport(const RHIViewport& viewport)
	{
		viewports.push_back(viewport);
		return *this;
	}

	RHIViewportState& addScissor(const RHIRect& scissor)
	{
		scissors.push_back(scissor);
		return *this;
	}
};


struct RHIColorBlendState {
	struct BlendDesc {
		bool           blend_enable{false};
		RHIBlendFactor src_blend{RHIBlendFactor::One};
		RHIBlendFactor dst_blend{RHIBlendFactor::Zero};
		RHIBlendOp     color_blend_op{RHIBlendOp::Add};
		RHIBlendFactor src_blend_alpha{RHIBlendFactor::One};
		RHIBlendFactor dst_blend_alpha{RHIBlendFactor::Zero};
		RHIBlendOp     alpha_blend_op{RHIBlendOp::Add};
		RHIColorMask   color_write_mask{RHIColorMask::All};

		BlendDesc& setBlendEnable(bool enabled) noexcept
		{
			blend_enable = enabled;
			return *this;
		}

		BlendDesc& setSrcBlend(RHIBlendFactor new_src_blend) noexcept
		{
			src_blend = new_src_blend;
			return *this;
		}

		BlendDesc& setDstBlend(RHIBlendFactor new_dst_blend) noexcept
		{
			dst_blend = new_dst_blend;
			return *this;
		}

		BlendDesc& setColorBlendOp(RHIBlendOp new_color_blend_op) noexcept
		{
			color_blend_op = new_color_blend_op;
			return *this;
		}

		BlendDesc& setSrcBlendAlpha(RHIBlendFactor new_src_blend_alpha) noexcept
		{
			src_blend_alpha = new_src_blend_alpha;
			return *this;
		}

		BlendDesc& setDstBlendAlpha(RHIBlendFactor new_dst_blend_alpha) noexcept
		{
			dst_blend_alpha = new_dst_blend_alpha;
			return *this;
		}

		BlendDesc& setAlphaBlendOp(RHIBlendOp new_alpha_blend_op) noexcept
		{
			alpha_blend_op = new_alpha_blend_op;
			return *this;
		}

		BlendDesc& setColorWriteMask(RHIColorMask new_color_write_mask) noexcept
		{
			color_write_mask = new_color_write_mask;
			return *this;
		}
	};

	std::vector<BlendDesc> blend_descs{};

	bool alpha_to_coverage_enable{};

	RHIColorBlendState& addBlendAttachment(const BlendDesc& blend_desc)
	{
		blend_descs.push_back(blend_desc);
		return *this;
	}

	RHIColorBlendState& setAlphaToCoverageEnable(bool enabled) noexcept
	{
		alpha_to_coverage_enable = enabled;
		return *this;
	}
};


struct RHIDepthStencilState {
	struct StencilOp {
		RHIStencilOp fail_op{RHIStencilOp::Keep};
		RHIStencilOp depth_fail_op{RHIStencilOp::Keep};
		RHIStencilOp pass_op{RHIStencilOp::Keep};
		RHICompareOp stencil_func{RHICompareOp::Always};

		StencilOp& setFailOp(RHIStencilOp new_fail_op) noexcept
		{
			fail_op = new_fail_op;
			return *this;
		}

		StencilOp& setDepthFailOp(RHIStencilOp new_depth_fail_op) noexcept
		{
			depth_fail_op = new_depth_fail_op;
			return *this;
		}

		StencilOp& setPassOp(RHIStencilOp new_pass_op) noexcept
		{
			pass_op = new_pass_op;
			return *this;
		}

		StencilOp& setStencilFunc(RHICompareOp new_stencil_func) noexcept
		{
			stencil_func = new_stencil_func;
			return *this;
		}
	};

	bool         depth_test_enable{false};
	bool         depth_write_enable{false};
	RHICompareOp depth_compare_op{RHICompareOp::Never};
	bool         depth_bounds_test_enable{false};
	bool         stencil_test_enable{false};
	StencilOp    front_face_stencil{};
	StencilOp    back_face_stencil{};

	RHIDepthStencilState& setDepthTestEnable(bool enabled) noexcept
	{
		depth_test_enable = enabled;
		return *this;
	}

	RHIDepthStencilState& setDepthWriteEnable(bool enabled) noexcept
	{
		depth_write_enable = enabled;
		return *this;
	}

	RHIDepthStencilState& setDepthCompareOp(RHICompareOp new_depth_compare_op) noexcept
	{
		depth_compare_op = new_depth_compare_op;
		return *this;
	}

	RHIDepthStencilState& setDepthBoundsTestEnable(bool enabled) noexcept
	{
		depth_bounds_test_enable = enabled;
		return *this;
	}

	RHIDepthStencilState& setStencilTestEnable(bool enabled) noexcept
	{
		stencil_test_enable = enabled;
		return *this;
	}

	RHIDepthStencilState& setFrontFaceStencil(const StencilOp& new_front_face_stencil) noexcept
	{
		front_face_stencil = new_front_face_stencil;
		return *this;
	}

	RHIDepthStencilState& setBackFaceStencil(const StencilOp& new_back_face_stencil) noexcept
	{
		back_face_stencil = new_back_face_stencil;
		return *this;
	}
};


struct RHIRasterState {
	RHIPolygonMode polygon_mode{RHIPolygonMode::Fill};
	RHICullMode    cull_mode{RHICullMode::Back};
	RHIFrontFace   front_face{RHIFrontFace::CounterClockwise};
	bool           depth_clamp_enable{false};
	bool           rasterizer_discard_enable{false};
	int            depth_bias{};
	float          depth_bias_clamp{};
	float          depth_bias_slope_factor{};

	RHIRasterState& setPolygonMode(RHIPolygonMode new_polygon_mode) noexcept
	{
		polygon_mode = new_polygon_mode;
		return *this;
	}

	RHIRasterState& setCullMode(RHICullMode new_cull_mode) noexcept
	{
		cull_mode = new_cull_mode;
		return *this;
	}

	RHIRasterState& setFrontFace(RHIFrontFace new_front_face) noexcept
	{
		front_face = new_front_face;
		return *this;
	}

	RHIRasterState& setDepthClampEnable(bool enabled) noexcept
	{
		depth_clamp_enable = enabled;
		return *this;
	}

	RHIRasterState& setRasterizerDiscardEnable(bool enabled) noexcept
	{
		rasterizer_discard_enable = enabled;
		return *this;
	}

	RHIRasterState& setDepthBias(int new_depth_bias) noexcept
	{
		depth_bias = new_depth_bias;
		return *this;
	}

	RHIRasterState& setDepthBiasClamp(float new_depth_bias_clamp) noexcept
	{
		depth_bias_clamp = new_depth_bias_clamp;
		return *this;
	}

	RHIRasterState& setDepthBiasSlopeFactor(float new_depth_bias_slope_factor) noexcept
	{
		depth_bias_slope_factor = new_depth_bias_slope_factor;
		return *this;
	}
};


struct RHIGraphicsPipelineDesc {
	RHIPrimitiveType primitive_type{RHIPrimitiveType::TriangleList};

	RHIRef<RHIInputLayout> input_layout{};
	RHIRef<RHIShader>      vertex_shader{};
	RHIRef<RHIShader>      pixel_shader{};

	RHIColorBlendState   blend_state{};
	RHIDepthStencilState depth_state{};
	RHIRasterState       raster_state{};
	RHIFramebufferInfo   framebuffer_info{};

	std::vector<RHIRef<RHIBindingLayout>> binding_layouts{};

	RHIGraphicsPipelineDesc& setPrimitiveType(RHIPrimitiveType new_primitive_type) noexcept
	{
		primitive_type = new_primitive_type;
		return *this;
	}

	RHIGraphicsPipelineDesc& setInputLayout(RHIInputLayout& new_input_layout) noexcept
	{
		input_layout = &new_input_layout;
		return *this;
	}

	RHIGraphicsPipelineDesc& setVertexShader(RHIShader& new_vertex_shader) noexcept
	{
		vertex_shader = &new_vertex_shader;
		return *this;
	}

	RHIGraphicsPipelineDesc& setPixelShader(RHIShader& new_pixel_shader) noexcept
	{
		pixel_shader = &new_pixel_shader;
		return *this;
	}

	RHIGraphicsPipelineDesc& setBlendState(const RHIColorBlendState& new_blend_state)
	{
		blend_state = new_blend_state;
		return *this;
	}

	RHIGraphicsPipelineDesc& setDepthStencilState(const RHIDepthStencilState& new_depth_state) noexcept
	{
		depth_state = new_depth_state;
		return *this;
	}

	RHIGraphicsPipelineDesc& setRasterState(const RHIRasterState& new_raster_state) noexcept
	{
		raster_state = new_raster_state;
		return *this;
	}

	RHIGraphicsPipelineDesc& setFramebufferInfo(const RHIFramebufferInfo& new_info)
	{
		framebuffer_info = new_info;
		return *this;
	}

	RHIGraphicsPipelineDesc& addBindingLayout(RHIBindingLayout& new_binding_layout)
	{
		binding_layouts.emplace_back(&new_binding_layout);
		return *this;
	}
};

class RHIGraphicsPipeline : public RHIResource {
public:
	virtual const RHIGraphicsPipelineDesc& getDesc() const noexcept = 0;
};

struct RHIComputePipelineDesc {
	RHIRef<RHIShader> compute_shader{};

	std::vector<RHIRef<RHIBindingLayout>> binding_layouts{};

	RHIComputePipelineDesc& setComputeShader(RHIShader& shader) noexcept
	{
		compute_shader = &shader;
		return *this;
	}

	RHIComputePipelineDesc& addBindingLayout(RHIBindingLayout& layout)
	{
		binding_layouts.emplace_back(&layout);
		return *this;
	}
};

class RHIComputePipeline : public RHIResource {
public:
	virtual const RHIComputePipelineDesc& getDesc() const noexcept = 0;
};

}        // namespace Vortex
