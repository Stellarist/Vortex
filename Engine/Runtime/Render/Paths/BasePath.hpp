#pragma once

#include "Runtime/Render/Backend/VulkanContext.hpp"

enum class PathType : uint32_t {
	Base = 0,
	Forward,
	Deferred,
	Count
};

class BasePath {
protected:
	PathType type{};

	VulkanContext* context{};

public:
	BasePath();
	virtual ~BasePath() = default;

	virtual void initialize(VulkanContext& context) = 0;
	virtual void cleanup() = 0;
	virtual void resize(uint32_t width, uint32_t height) = 0;
};
