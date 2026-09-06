module Runtime.Render;

namespace Vortex {

BlitPass::BlitPass(BlitPassParams pass_parameters) :
    RDGPass(RDGPassDesc{.name = "Blit", .type = RDGPassType::Copy}),
    parameters(std::move(pass_parameters))
{
	CHECK(
	    parameters.source && parameters.destination &&
	        parameters.source != parameters.destination,
	    "Blit pass requires distinct source and destination textures");
}

std::unique_ptr<BlitPass> BlitPass::create(BlitPassParams parameters)
{
	return std::unique_ptr<BlitPass>(
	    new BlitPass(std::move(parameters)));
}

void BlitPass::setup(RDGPassBuilder& builder)
{
	builder.read(parameters.source, CopySource);
	builder.write(parameters.destination, CopyDest);
}

void BlitPass::execute(RDGPassContext& context)
{
	context.getCommand().copyTexture(
	    &context.getTexture(parameters.destination),
	    {},
	    &context.getTexture(parameters.source),
	    {});
}

}        // namespace Vortex
