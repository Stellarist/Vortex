export module Runtime.Graphics:RHIContext;

import :RHITypes;

export namespace Vortex {

class RHIBindingSet;
class RHIBuffer;
class RHICommandList;
class RHIDevice;
class RHITexture;
class RHITextureView;

struct RHIContextDesc {
	RHIAPI    api{RHIAPI::Vulkan};
	RHIExtent extent{2560, 1440};
	RHIFormat format{RHIFormat::RGBA8_SRGB};
};

class RHIContext {
public:
	RHIContext() = default;
	virtual ~RHIContext() noexcept = default;

	RHIContext(const RHIContext&) = delete;
	RHIContext& operator=(const RHIContext&) = delete;
	RHIContext(RHIContext&&) = delete;
	RHIContext& operator=(RHIContext&&) = delete;

	virtual RHIDevice& getDevice() noexcept = 0;
	virtual RHIExtent  getExtent() const noexcept = 0;
	virtual RHIFormat  getFormat() const noexcept = 0;

	virtual void beginFrame() = 0;
	virtual void endFrame() = 0;

	virtual RHICommandList& getCommand() noexcept = 0;
	virtual RHITexture&     getBackbuffer() noexcept = 0;
	virtual RHITextureView& getBackbufferView() noexcept = 0;
};

}        // namespace Vortex
