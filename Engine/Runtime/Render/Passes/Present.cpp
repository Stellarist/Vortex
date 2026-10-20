module Runtime.Render;

namespace Vortex {

void PresentPass::setup(RDGPassBuilder& builder, Data& data)
{
	builder.read(data.source, TransferSource);
	builder.write(data.destination, TransferDest);
}

void PresentPass::execute(RDGPassContext& context, const Data& data)
{
	context.getCommand().copyTexture(&context.getTexture(data.destination), {},
	    &context.getTexture(data.source), {});
}

void PresentPass::build(RDGBuilder& graph, const Params& params) const
{
	graph.addPass(RDGPassDesc{.name = "Present.Copy", .type = RDGPassType::Copy},
	    Data{.source = params.color, .destination = params.output}, setup, execute);
}

}        // namespace Vortex
