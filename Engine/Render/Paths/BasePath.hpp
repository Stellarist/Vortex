#pragma once

#include "Render/Graphics/Context.hpp"

enum class PathType : uint32_t {
	Base = 0,
	Forward,
	Deferred,
	Count
};

class BasePath {
protected:
	PathType type{};

	Context* context{};

public:
	BasePath();
	virtual ~BasePath() = default;

	virtual void initialize(Context& context) = 0;
	virtual void cleanup() = 0;
	virtual void resize(uint32_t width, uint32_t height) = 0;
};
