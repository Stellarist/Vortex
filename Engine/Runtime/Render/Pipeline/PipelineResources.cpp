module Runtime.Render;

namespace Vortex {

static constexpr std::array SHADER_FILES{
    "ShadowDepth.spv",
    "Forward.spv",
    "Geometry.spv",
    "Deferred.spv",
};

PipelineResources::PipelineResources(RHIDevice& new_device) :
    device(&new_device)
{
	mesh_input_layout = device->createInputLayout(RHIInputLayoutDesc{}
	        .addBindingDesc(MeshResource::vertexBinding())
	        .setAttributeDescs(MeshResource::vertexAttributes()));
	mesh_input_layout->setName("Render.Mesh.InputLayout");

	auto shadow_attributes = MeshResource::vertexAttributes();
	shadow_input_layout = device->createInputLayout(RHIInputLayoutDesc{}
	        .addBindingDesc(MeshResource::vertexBinding())
	        .addAttributeDesc(shadow_attributes.front()));
	shadow_input_layout->setName("Render.Shadow.InputLayout");

	point_clamp_sampler = device->createSampler(RHISamplerDesc{}
	        .setAllFilters(false)
	        .setAllAddressModes(RHISamplerAddress::ClampToEdge));
	point_clamp_sampler->setName("Render.PointClampSampler");

	shadow_sample_layout = device->createBindingLayout(RHIBindingLayoutDesc{}
	        .setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel)
	        .addItem(RHIBindingLayoutItem::constantBuffer(0))
	        .addItem(RHIBindingLayoutItem::textureSRV(1))
	        .addItem(RHIBindingLayoutItem::sampler(2)));
	shadow_sample_layout->setName("Render.ShadowSample.BindingLayout");

	gbuffer_sample_layout = device->createBindingLayout(RHIBindingLayoutDesc{}
	        .setVisibility(RHIShaderType::Vertex | RHIShaderType::Pixel)
	        .addItem(RHIBindingLayoutItem::textureSRV(0))
	        .addItem(RHIBindingLayoutItem::textureSRV(1))
	        .addItem(RHIBindingLayoutItem::textureSRV(2))
	        .addItem(RHIBindingLayoutItem::textureSRV(3))
	        .addItem(RHIBindingLayoutItem::sampler(4)));
	gbuffer_sample_layout->setName("Render.GBufferSample.BindingLayout");
}

RHIShader& PipelineResources::getShader(Shader shader_id, RHIShaderType stage)
{
	const auto index = static_cast<size_t>(shader_id);
	CHECK(Range, index < shaders.size(), "Invalid render shader id");
	CHECK(Argument, stage == RHIShaderType::Vertex || stage == RHIShaderType::Pixel,
	    "Render shaders only provide vertex and pixel stages");

	auto& shader = shaders[index][stage == RHIShaderType::Vertex ? 0 : 1];
	if (shader)
		return *shader;

	const auto path = File::shadersDir() / SHADER_FILES[index];
	const auto bytecode = File::readBinaryFile(path);
	shader = device->createShader(RHIShaderDesc{}
	                                  .setType(stage)
	                                  .setShaderName(path.stem().string())
	                                  .setEntryPoint(stage == RHIShaderType::Vertex ? "vertexMain" : "pixelMain"),
	    bytecode);
	return *shader;
}

RHIGraphicsPipeline* PipelineResources::findPipeline(PipelineType type,
    const RHIFramebufferInfo& framebuffer,
    const std::array<const RHIBindingLayout*, 4>& layouts) const noexcept
{
	for (const auto& entry : pipelines)
		if (entry.type == type && entry.framebuffer == framebuffer &&
		    entry.layouts == layouts)
			return entry.pipeline.get();
	return nullptr;
}

RHIGraphicsPipeline& PipelineResources::storePipeline(PipelineType type,
    const RHIFramebufferInfo& framebuffer,
    std::array<const RHIBindingLayout*, 4> layouts,
    RHIRef<RHIGraphicsPipeline> pipeline)
{
	CHECK(pipeline, "Cannot cache an empty graphics pipeline");
	pipelines.push_back({type, framebuffer, layouts, std::move(pipeline)});
	return *pipelines.back().pipeline;
}

RHIGraphicsPipeline& PipelineResources::getForwardPipeline(
    const RHIFramebufferInfo& framebuffer, const RenderFrame& frame,
    bool transparent)
{
	const auto type = transparent ?
	    PipelineType::ForwardTransparent :
	    PipelineType::ForwardOpaque;
	const std::array<const RHIBindingLayout*, 4> layouts{
	    frame.getSceneLayout(), frame.getMaterialLayout(),
	    frame.getObjectLayout(), shadow_sample_layout.get()};
	if (auto* pipeline = findPipeline(type, framebuffer, layouts))
		return *pipeline;

	RHIGraphicsPipelineDesc desc{};
	desc.setInputLayout(*mesh_input_layout)
	    .setRasterState(RHIRasterState{}.setCullMode(RHICullMode::None))
	    .setDepthStencilState(RHIDepthStencilState{}
	            .setDepthTestEnable(true)
	            .setDepthWriteEnable(!transparent)
	            .setDepthCompareOp(RHICompareOp::LessOrEqual))
	    .setFramebufferInfo(framebuffer)
	    .setVertexShader(getShader(Shader::Forward, RHIShaderType::Vertex))
	    .setPixelShader(getShader(Shader::Forward, RHIShaderType::Pixel))
	    .addBindingLayout(*frame.getSceneLayout())
	    .addBindingLayout(*frame.getMaterialLayout())
	    .addBindingLayout(*frame.getObjectLayout())
	    .addBindingLayout(*shadow_sample_layout);
	if (transparent) {
		const auto blend = RHIColorBlendState::BlendDesc{}
		                       .setBlendEnable(true)
		                       .setSrcBlend(RHIBlendFactor::SrcAlpha)
		                       .setDstBlend(RHIBlendFactor::OneMinusSrcAlpha)
		                       .setSrcBlendAlpha(RHIBlendFactor::One)
		                       .setDstBlendAlpha(RHIBlendFactor::OneMinusSrcAlpha);
		desc.setBlendState(RHIColorBlendState{}.addBlendAttachment(blend));
	}
	auto pipeline = device->createGraphicsPipeline(desc);
	pipeline->setName(transparent ?
	        "Forward.Transparent.GraphicsPipeline" :
	        "Forward.Opaque.GraphicsPipeline");
	return storePipeline(type, framebuffer, layouts, std::move(pipeline));
}

RHIGraphicsPipeline& PipelineResources::getGBufferPipeline(
    const RHIFramebufferInfo& framebuffer, const RenderFrame& frame)
{
	const std::array<const RHIBindingLayout*, 4> layouts{
	    frame.getSceneLayout(), frame.getMaterialLayout(),
	    frame.getObjectLayout(), nullptr};
	if (auto* pipeline = findPipeline(PipelineType::GBuffer, framebuffer, layouts))
		return *pipeline;

	RHIGraphicsPipelineDesc desc{};
	desc.setInputLayout(*mesh_input_layout)
	    .setRasterState(RHIRasterState{}.setCullMode(RHICullMode::None))
	    .setDepthStencilState(RHIDepthStencilState{}
	            .setDepthTestEnable(true)
	            .setDepthWriteEnable(true)
	            .setDepthCompareOp(RHICompareOp::LessOrEqual))
	    .setFramebufferInfo(framebuffer)
	    .setVertexShader(getShader(Shader::Geometry, RHIShaderType::Vertex))
	    .setPixelShader(getShader(Shader::Geometry, RHIShaderType::Pixel))
	    .addBindingLayout(*frame.getSceneLayout())
	    .addBindingLayout(*frame.getMaterialLayout())
	    .addBindingLayout(*frame.getObjectLayout());
	auto pipeline = device->createGraphicsPipeline(desc);
	pipeline->setName("Deferred.GBuffer.GraphicsPipeline");
	return storePipeline(PipelineType::GBuffer, framebuffer, layouts,
	    std::move(pipeline));
}

RHIGraphicsPipeline& PipelineResources::getDeferredPipeline(
    const RHIFramebufferInfo& framebuffer, const RenderFrame& frame)
{
	const std::array<const RHIBindingLayout*, 4> layouts{
	    frame.getSceneLayout(), gbuffer_sample_layout.get(),
	    shadow_sample_layout.get(), nullptr};
	if (auto* pipeline = findPipeline(PipelineType::Deferred, framebuffer, layouts))
		return *pipeline;

	RHIGraphicsPipelineDesc desc{};
	desc.setRasterState(RHIRasterState{}.setCullMode(RHICullMode::None))
	    .setFramebufferInfo(framebuffer)
	    .setVertexShader(getShader(Shader::Deferred, RHIShaderType::Vertex))
	    .setPixelShader(getShader(Shader::Deferred, RHIShaderType::Pixel))
	    .addBindingLayout(*frame.getSceneLayout())
	    .addBindingLayout(*gbuffer_sample_layout)
	    .addBindingLayout(*shadow_sample_layout);
	auto pipeline = device->createGraphicsPipeline(desc);
	pipeline->setName("Deferred.Lighting.GraphicsPipeline");
	return storePipeline(PipelineType::Deferred, framebuffer, layouts,
	    std::move(pipeline));
}

RHIGraphicsPipeline& PipelineResources::getShadowPipeline(
    const RHIFramebufferInfo& framebuffer, const RenderFrame& frame,
    const RHIBindingLayout& shadow_draw_layout)
{
	const std::array<const RHIBindingLayout*, 4> layouts{
	    &shadow_draw_layout, frame.getObjectLayout(), nullptr, nullptr};
	if (auto* pipeline = findPipeline(PipelineType::Shadow, framebuffer, layouts))
		return *pipeline;

	RHIGraphicsPipelineDesc desc{};
	desc.setInputLayout(*shadow_input_layout)
	    .setRasterState(RHIRasterState{}
	            .setCullMode(RHICullMode::Back)
	            .setDepthBias(1)
	            .setDepthBiasSlopeFactor(1.5f))
	    .setDepthStencilState(RHIDepthStencilState{}
	            .setDepthTestEnable(true)
	            .setDepthWriteEnable(true)
	            .setDepthCompareOp(RHICompareOp::LessOrEqual))
	    .setFramebufferInfo(framebuffer)
	    .setVertexShader(getShader(Shader::ShadowDepth, RHIShaderType::Vertex))
	    .setPixelShader(getShader(Shader::ShadowDepth, RHIShaderType::Pixel))
	    .addBindingLayout(const_cast<RHIBindingLayout&>(shadow_draw_layout))
	    .addBindingLayout(*frame.getObjectLayout());
	auto pipeline = device->createGraphicsPipeline(desc);
	pipeline->setName("Shadow.DepthPipeline");
	return storePipeline(PipelineType::Shadow, framebuffer, layouts,
	    std::move(pipeline));
}

}        // namespace Vortex
