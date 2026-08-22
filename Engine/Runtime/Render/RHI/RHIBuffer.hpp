#pragma once

#include <cstdint>

#include "RHIResource.hpp"
#include "RHITypes.hpp"

struct RHIBufferDesc {
	uint64_t size{};
	uint32_t stride{};

	RHIFormat      format{RHIFormat::Unknown};
	RHIBufferUsage usage{RHIBufferUsage::None};
	RHIAccessMode  access{RHIAccessMode::None};

	RHIBufferDesc& setSize(uint64_t new_size) noexcept
	{
		size = new_size;
		return *this;
	}

	RHIBufferDesc& setStride(uint32_t new_stride) noexcept
	{
		stride = new_stride;
		return *this;
	}

	RHIBufferDesc& setUsage(RHIBufferUsage new_usage) noexcept
	{
		usage = new_usage;
		return *this;
	}

	RHIBufferDesc& setAccess(RHIAccessMode new_access) noexcept
	{
		access = new_access;
		return *this;
	}
};

class RHIBuffer : public RHIResource {
public:
	virtual const RHIBufferDesc& getDesc() const noexcept = 0;
};


struct RHIBufferViewDesc {
	RHIRef<RHIBuffer> buffer{};
	RHIBufferViewType type{RHIBufferViewType::Constant};
	RHIFormat         format{RHIFormat::Unknown};
	uint64_t          offset{};
	uint64_t          size{};
	uint32_t          stride{};

	RHIBufferViewDesc& setBuffer(RHIBuffer* new_buffer) noexcept
	{
		buffer = new_buffer;
		return *this;
	}

	RHIBufferViewDesc& setType(RHIBufferViewType new_type) noexcept
	{
		type = new_type;
		return *this;
	}

	RHIBufferViewDesc& setFormat(RHIFormat new_format) noexcept
	{
		format = new_format;
		return *this;
	}

	RHIBufferViewDesc& setRange(uint64_t new_offset, uint64_t new_size = 0) noexcept
	{
		offset = new_offset;
		size = new_size;
		return *this;
	}

	RHIBufferViewDesc& setStride(uint32_t new_stride) noexcept
	{
		stride = new_stride;
		return *this;
	}
};

class RHIBufferView : public RHIResource {
public:
	virtual const RHIBufferViewDesc& getDesc() const noexcept = 0;
	virtual RHIBuffer&               getBuffer() const noexcept = 0;
};
