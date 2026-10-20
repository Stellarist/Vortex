export module Runtime.Render:Present;

import Core;
import Runtime.RHI;
import Runtime.RDG;
import :Core;

namespace Vortex {

class PresentPass {
public:
	struct Params {
		RDGTextureRef color{};
		RDGTextureRef output{};
	};

private:
	struct Data {
		RDGTextureRef source{};
		RDGTextureRef destination{};
	};

	static void setup(RDGPassBuilder& builder, Data& data);
	static void execute(RDGPassContext& context, const Data& data);

public:
	void build(RDGBuilder&, const Params&) const;
};

}        // namespace Vortex
