export module Runtime.Object;

import Core;

export namespace Vortex {

class Object {
private:
	std::string name;
	uint64      uid;

	static std::atomic<uint64> id_counter;

public:
	Object(std::string name = {}) noexcept;
	virtual ~Object() noexcept = default;

	Object(const Object&) = delete;
	Object& operator=(const Object&) = delete;
	Object(Object&&) noexcept = delete;
	Object& operator=(Object&&) noexcept = delete;

	uint64 getUid() const noexcept;
	auto   getName() const noexcept -> const std::string&;

	template <typename Self>
	auto&& setName(this Self&& self, std::string name)
	{
		self.name = std::move(name);
		return std::forward<Self>(self);
	}

	std::type_index getType() const noexcept;
};

}        // namespace Vortex
