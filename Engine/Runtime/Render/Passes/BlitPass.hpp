export module Runtime.Render:Pass.Blit;

import Core;
import :Pass;
import Runtime.RDG;

export namespace Vortex {

struct BlitPassParams {
	RDGTextureRef source{};
	RDGTextureRef destination{};
};

class BlitPass final : public RDGPass {
private:
	BlitPassParams parameters{};

	BlitPass(BlitPassParams pass_parameters);

	void setup(RDGPassBuilder& builder) override;
	void execute(RDGPassContext& context) override;

public:
	static std::unique_ptr<BlitPass> create(
	    BlitPassParams parameters);
};

}        // namespace Vortex
