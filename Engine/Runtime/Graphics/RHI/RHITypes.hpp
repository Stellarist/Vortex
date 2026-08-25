export module Runtime.Graphics:RHI.Types;

import Core;

export namespace Vortex {

template <typename E>
struct EnableEnumFlags : std::false_type {};

template <typename E>
concept EnumFlags =
    std::is_enum_v<E> && EnableEnumFlags<E>::value;

template <EnumFlags E>
constexpr E operator|(E lhs, E rhs) noexcept
{
	using U = std::underlying_type_t<E>;
	return static_cast<E>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

template <EnumFlags E>
constexpr E operator&(E lhs, E rhs) noexcept
{
	using U = std::underlying_type_t<E>;
	return static_cast<E>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

template <EnumFlags E>
constexpr E operator^(E lhs, E rhs) noexcept
{
	using U = std::underlying_type_t<E>;
	return static_cast<E>(static_cast<U>(lhs) ^ static_cast<U>(rhs));
}

template <EnumFlags E>
constexpr E operator~(E value) noexcept
{
	using U = std::underlying_type_t<E>;
	return static_cast<E>(~static_cast<U>(value));
}

template <EnumFlags E>
constexpr E& operator|=(E& lhs, E rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

template <EnumFlags E>
constexpr E& operator&=(E& lhs, E rhs) noexcept
{
	lhs = lhs & rhs;
	return lhs;
}

template <EnumFlags E>
constexpr E& operator^=(E& lhs, E rhs) noexcept
{
	lhs = lhs ^ rhs;
	return lhs;
}


enum class RHIAPI : uint8 {
	Vulkan,
};

enum class RHIFormat : uint16 {
	Unknown,
	R8_UINT,
	R8_SINT,
	R8_UNORM,
	R8_SNORM,
	RG8_UINT,
	RG8_SINT,
	RG8_UNORM,
	RG8_SNORM,
	R16_UINT,
	R16_SINT,
	R16_UNORM,
	R16_SNORM,
	R16_FLOAT,
	RGBA8_UINT,
	RGBA8_SINT,
	RGBA8_UNORM,
	RGBA8_SRGB,
	RGBA8_SNORM,
	BGRA8_UNORM,
	BGRA8_SRGB,
	RG16_UINT,
	RG16_SINT,
	RG16_UNORM,
	RG16_SNORM,
	RG16_FLOAT,
	R32_UINT,
	R32_SINT,
	R32_FLOAT,
	RGBA16_UINT,
	RGBA16_SINT,
	RGBA16_FLOAT,
	RGBA16_UNORM,
	RGBA16_SNORM,
	RG32_UINT,
	RG32_SINT,
	RG32_FLOAT,
	RGB32_UINT,
	RGB32_SINT,
	RGB32_FLOAT,
	RGBA32_UINT,
	RGBA32_SINT,
	RGBA32_FLOAT,
	D16_UNORM,
	D24_UNORM_S8_UINT,
	D32_FLOAT,
};

enum RHIResourceState : uint16 {
	Unknown = 0,
	Common = 1 << 0,
	ConstantBuffer = 1 << 1,
	VertexBuffer = 1 << 2,
	IndexBuffer = 1 << 3,
	IndirectBuffer = 1 << 4,
	RenderTarget = 1 << 5,
	DepthWrite = 1 << 6,
	DepthRead = 1 << 7,
	ShaderResource = 1 << 8,
	UnorderedAccess = 1 << 9,
	CopySource = 1 << 10,
	CopyDest = 1 << 11,
	Present = 1 << 12,
};

enum class RHIBindingType : uint8 {
	None,
	TextureSRV,
	TextureUAV,
	TypedBufferSRV,
	TypedBufferUAV,
	StructuredBufferSRV,
	StructuredBufferUAV,
	RawBufferSRV,
	RawBufferUAV,
	ConstantBuffer,
	PushConstants,
	Sampler,
};

enum class RHIBufferViewType : uint8 {
	Constant,
	Typed,
	Structured,
	Raw,
};

enum class RHITextureViewType : uint8 {
	ShaderResource,
	UnorderedAccess,
	RenderTarget,
	DepthStencil,
};

enum class RHITextureViewDimension : uint8 {
	Automatic,
	Texture1D,
	Texture1DArray,
	Texture2D,
	Texture2DArray,
	Texture3D,
	TextureCube,
	TextureCubeArray,
};

enum class RHIColorMask : uint8 {
	None = 0,
	R = 1 << 0,
	G = 1 << 1,
	B = 1 << 2,
	A = 1 << 3,
	All = R | G | B | A,
};

enum class RHIIndexType : uint8 {
	Uint8,
	Uint16,
	Uint32,
};

enum class RHIPrimitiveType : uint8 {
	TriangleList,
	TriangleStrip,
	LineList,
	LineStrip,
	PointList,
};

enum class RHICompareOp : uint8 {
	Never,
	Less,
	LessOrEqual,
	Greater,
	NotEqual,
	GreaterOrEqual,
	Always,
};

enum class RHIBlendFactor : uint8 {
	Zero,
	One,
	SrcColor,
	OneMinusSrcColor,
	DstColor,
	OneMinusDstColor,
	SrcAlpha,
	OneMinusSrcAlpha,
	DstAlpha,
	OneMinusDstAlpha,
};

enum class RHIBlendOp : uint8 {
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max,
};

enum class RHIStencilOp : uint8 {
	Keep,
	Zero,
	Replace,
	IncrementAndClamp,
	DecrementAndClamp,
	Invert,
	IncrementAndWrap,
	DecrementAndWrap,
};

enum class RHIPolygonMode : uint8 {
	Fill,
	Line,
	Point,
};

enum class RHICullMode : uint8 {
	None,
	Front,
	Back,
};

enum class RHIFrontFace : uint8 {
	CounterClockwise,
	Clockwise,
};

enum class RHITextureDimension : uint8 {
	Texture1D,
	Texture2D,
	Texture3D,
	TextureCube,
};

enum class RHIBufferUsage : uint16 {
	None = 0,
	VertexBuffer = 1 << 0,
	IndexBuffer = 1 << 1,
	ConstantBuffer = 1 << 2,
	StorageBuffer = 1 << 3,
	IndirectArgument = 1 << 4,
	CopySource = 1 << 5,
	CopyDest = 1 << 6,
	TypedBuffer = 1 << 7,
};

enum class RHITextureUsage : uint8 {
	None = 0,
	Sampled = 1 << 0,
	RenderTarget = 1 << 1,
	DepthStencil = 1 << 2,
	Storage = 1 << 3,
	CopySource = 1 << 4,
	CopyDest = 1 << 5,
};

enum class RHISamplerAddress : uint8 {
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder,
};

enum class RHIShaderType : uint8 {
	None = 0,
	Vertex = 1 << 0,
	Pixel = 1 << 1,
	Geometry = 1 << 2,
	Compute = 1 << 3,
	AllGraphics = (1 << 0) | (1 << 1) | (1 << 2),
	All = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3),
};

enum class RHIFilter : uint8 {
	Linear,
	Nearest,
};

enum class RHIAccessMode : uint8 {
	None,
	Read,
	Write,
};

enum class RHICommandQueue : uint8 {
	Graphics,
	Compute,
	Copy,
	Present,
};


template <>
struct EnableEnumFlags<RHIResourceState> : std::true_type {};

template <>
struct EnableEnumFlags<RHIColorMask> : std::true_type {};

template <>
struct EnableEnumFlags<RHIBufferUsage> : std::true_type {};

template <>
struct EnableEnumFlags<RHITextureUsage> : std::true_type {};

template <>
struct EnableEnumFlags<RHIShaderType> : std::true_type {};


struct RHIColor {
	float r{};
	float g{};
	float b{};
	float a{1.0f};

	RHIColor() = default;
	RHIColor(float c) noexcept : r(c), g(c), b(c), a(1.0f) {}
	RHIColor(float r, float g, float b, float a = 1.0f) noexcept : r(r), g(g), b(b), a(a) {}

	bool operator==(const RHIColor&) const = default;
	bool operator!=(const RHIColor&) const = default;

	RHIColor& set(float c) noexcept
	{
		r = g = b = c;
		a = 1.0f;
		return *this;
	}

	RHIColor& setR(float new_r) noexcept
	{
		r = new_r;
		return *this;
	}

	RHIColor& setG(float new_g) noexcept
	{
		g = new_g;
		return *this;
	}

	RHIColor& setB(float new_b) noexcept
	{
		b = new_b;
		return *this;
	}

	RHIColor& setA(float new_a) noexcept
	{
		a = new_a;
		return *this;
	}
};

struct RHIExtent {
	uint32 width{};
	uint32 height{};

	RHIExtent() = default;
	RHIExtent(uint32 width, uint32 height) noexcept : width(width), height(height) {}

	bool operator==(const RHIExtent&) const = default;
	bool operator!=(const RHIExtent&) const = default;

	RHIExtent& setWidth(uint32 new_width) noexcept
	{
		width = new_width;
		return *this;
	}

	RHIExtent& setHeight(uint32 new_height) noexcept
	{
		height = new_height;
		return *this;
	}
};

struct RHIViewport {
	float x_min{}, x_max{};
	float y_min{}, y_max{};
	float z_min{}, z_max{1.0f};

	RHIViewport() = default;
	RHIViewport(float width, float height) noexcept : x_min(0.0f), x_max(width), y_min(0.0f), y_max(height), z_min(0.0f), z_max(1.0f) {}
	RHIViewport(float x_min, float x_max, float y_min, float y_max, float z_min = 0.0f, float z_max = 1.0f) noexcept :
	    x_min(x_min), x_max(x_max), y_min(y_min), y_max(y_max), z_min(z_min), z_max(z_max) {}

	bool operator==(const RHIViewport&) const = default;
	bool operator!=(const RHIViewport&) const = default;

	RHIViewport& setX(float new_x_min, float new_x_max) noexcept
	{
		x_min = new_x_min;
		x_max = new_x_max;
		return *this;
	}

	RHIViewport& setY(float new_y_min, float new_y_max) noexcept
	{
		y_min = new_y_min;
		y_max = new_y_max;
		return *this;
	}

	RHIViewport& setZ(float new_z_min, float new_z_max) noexcept
	{
		z_min = new_z_min;
		z_max = new_z_max;
		return *this;
	}

	float width() const noexcept { return x_max - x_min; }
	float height() const noexcept { return y_max - y_min; }
};

struct RHIClearValue {
	RHIColor color{};
	float    depth{1.0f};
	uint32   stencil{};

	RHIClearValue() = default;
	RHIClearValue(RHIColor clear_color, float clear_depth = 1.0f, uint32 clear_stencil = 0) noexcept : color(clear_color), depth(clear_depth), stencil(clear_stencil) {}

	bool operator==(const RHIClearValue&) const = default;
	bool operator!=(const RHIClearValue&) const = default;

	RHIClearValue& setColor(RHIColor new_color) noexcept
	{
		color = new_color;
		return *this;
	}

	RHIClearValue& setDepthStencil(float new_depth, uint32 new_stencil = 0) noexcept
	{
		depth = new_depth;
		stencil = new_stencil;
		return *this;
	}
};

struct RHIRect {
	int x_min{}, x_max{};
	int y_min{}, y_max{};

	RHIRect() = default;
	RHIRect(int width, int height) noexcept : x_min(0), x_max(width), y_min(0), y_max(height) {}
	RHIRect(int x_min, int x_max, int y_min, int y_max) noexcept : x_min(x_min), x_max(x_max), y_min(y_min), y_max(y_max) {}
	RHIRect(const RHIViewport& viewport) noexcept :
	    x_min(static_cast<int>(std::floorf(viewport.x_min))),
	    x_max(static_cast<int>(std::floorf(viewport.x_max))),
	    y_min(static_cast<int>(std::floorf(viewport.y_min))),
	    y_max(static_cast<int>(std::floorf(viewport.y_max))) {}

	bool operator==(const RHIRect&) const = default;
	bool operator!=(const RHIRect&) const = default;

	RHIRect& setX(int new_x_min, int new_x_max) noexcept
	{
		x_min = new_x_min;
		x_max = new_x_max;
		return *this;
	}

	RHIRect& setY(int new_y_min, int new_y_max) noexcept
	{
		y_min = new_y_min;
		y_max = new_y_max;
		return *this;
	}

	int width() const noexcept { return x_max - x_min; }
	int height() const noexcept { return y_max - y_min; }
};

struct RHITextureSlice {
	int x{}, y{}, z{};
	int width{-1}, height{-1}, depth{-1};

	uint32 mip_level{};
	uint32 array_layer{};

	RHITextureSlice& setOffset(int new_x, int new_y, int new_z = 0) noexcept
	{
		x = new_x;
		y = new_y;
		z = new_z;
		return *this;
	}

	RHITextureSlice& setExtent(int new_width, int new_height, int new_depth = 1) noexcept
	{
		width = new_width;
		height = new_height;
		depth = new_depth;
		return *this;
	}

	RHITextureSlice& setMipLevel(uint32 new_mip_level) noexcept
	{
		mip_level = new_mip_level;
		return *this;
	}

	RHITextureSlice& setArrayLayer(uint32 new_array_layer) noexcept
	{
		array_layer = new_array_layer;
		return *this;
	}
};

struct RHITextureSubresource {
	uint32 base_array_layer{};
	uint32 layer_count{};
	uint32 base_mip_level{};
	uint32 level_count{};

	RHITextureSubresource& setBaseArrayLayer(uint32 new_base_array_layer) noexcept
	{
		base_array_layer = new_base_array_layer;
		return *this;
	}

	RHITextureSubresource& setLayerCount(uint32 new_layer_count) noexcept
	{
		layer_count = new_layer_count;
		return *this;
	}

	RHITextureSubresource& setBaseMipLevel(uint32 new_base_mip_level) noexcept
	{
		base_mip_level = new_base_mip_level;
		return *this;
	}

	RHITextureSubresource& setLevelCount(uint32 new_level_count) noexcept
	{
		level_count = new_level_count;
		return *this;
	}
};

struct RHIDrawArguments {
	uint32 vertex_count{};
	uint32 instance_count{1};
	uint32 start_vertex{};
	uint32 start_index{};
	uint32 start_instance{};

	RHIDrawArguments& setVertexCount(uint32 new_vertex_count) noexcept
	{
		vertex_count = new_vertex_count;
		return *this;
	}

	RHIDrawArguments& setInstanceCount(uint32 new_instance_count) noexcept
	{
		instance_count = new_instance_count;
		return *this;
	}

	RHIDrawArguments& setStartVertex(uint32 new_start_vertex) noexcept
	{
		start_vertex = new_start_vertex;
		return *this;
	}

	RHIDrawArguments& setStartIndex(uint32 new_start_index) noexcept
	{
		start_index = new_start_index;
		return *this;
	}

	RHIDrawArguments& setStartInstance(uint32 new_start_instance) noexcept
	{
		start_instance = new_start_instance;
		return *this;
	}
};

}        // namespace Vortex
