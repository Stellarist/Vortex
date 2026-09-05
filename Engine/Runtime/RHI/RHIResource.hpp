export module Runtime.RHI:Resource;

import Core;

export namespace Vortex {

class RHIResource {
private:
	std::string name{};

	mutable std::atomic_uint32_t ref_count{1};

protected:
	RHIResource() = default;
	virtual ~RHIResource() noexcept = default;

	virtual void applyName(const std::string&) noexcept {}

public:
	RHIResource(const RHIResource&) = delete;
	RHIResource& operator=(const RHIResource&) = delete;

	RHIResource(RHIResource&&) = delete;
	RHIResource& operator=(RHIResource&&) = delete;

	uint32 addRef() const noexcept
	{
		return ref_count.fetch_add(1, std::memory_order_relaxed) + 1;
	}

	uint32 release() const noexcept
	{
		const uint32 previous_count = ref_count.fetch_sub(1, std::memory_order_acq_rel);
		if (previous_count == 0)
			ERROR(Logic, "RHI resource '{}' reference count underflow", name);
		if (previous_count == 1)
			delete this;
		return previous_count - 1;
	}

	uint32 getRefCount() const noexcept
	{
		return ref_count.load(std::memory_order_relaxed);
	}

	const std::string& getName() const noexcept
	{
		return name;
	}

	RHIResource& setName(std::string new_name)
	{
		name = std::move(new_name);
		applyName(name);
		return *this;
	}
};


template <typename T>
class RHIRef {
private:
	T* reference{};

	struct AdoptTag {};

	RHIRef(T* new_reference, AdoptTag) noexcept :
	    reference(new_reference) {}

	template <typename U>
	friend class RHIRef;

public:
	RHIRef() noexcept = default;
	RHIRef(std::nullptr_t) noexcept {}

	RHIRef(T* new_reference) noexcept :
	    reference(new_reference)
	{
		if (reference)
			reference->addRef();
	}

	RHIRef(const RHIRef& other) noexcept :
	    RHIRef(other.reference) {}

	template <typename U> requires std::is_convertible_v<U*, T*>
	RHIRef(const RHIRef<U>& other) noexcept :
	    RHIRef(other.reference)
	{}

	RHIRef(RHIRef&& other) noexcept :
	    reference(std::exchange(other.reference, nullptr)) {}

	template <typename U> requires std::is_convertible_v<U*, T*>
	RHIRef(RHIRef<U>&& other) noexcept :
	    reference(std::exchange(other.reference, nullptr))
	{}

	~RHIRef() noexcept
	{
		if (reference)
			reference->release();
	}

	RHIRef& operator=(const RHIRef& other) noexcept
	{
		reset(other.reference);
		return *this;
	}

	template <typename U> requires std::is_convertible_v<U*, T*>
	RHIRef& operator=(const RHIRef<U>& other) noexcept
	{
		reset(other.reference);
		return *this;
	}

	RHIRef& operator=(RHIRef&& other) noexcept
	{
		if (this != &other) {
			if (reference)
				reference->release();
			reference = std::exchange(other.reference, nullptr);
		}
		return *this;
	}

	template <typename U> requires std::is_convertible_v<U*, T*>
	RHIRef& operator=(RHIRef<U>&& other) noexcept
	{
		if (reference)
			reference->release();
		reference = std::exchange(other.reference, nullptr);
		return *this;
	}

	RHIRef& operator=(T* new_reference) noexcept
	{
		reset(new_reference);
		return *this;
	}

	RHIRef& operator=(std::nullptr_t) noexcept
	{
		reset();
		return *this;
	}

	static RHIRef adopt(T* new_reference) noexcept
	{
		return RHIRef(new_reference, AdoptTag{});
	}

	void reset(T* new_reference = nullptr) noexcept
	{
		if (reference == new_reference)
			return;
		if (new_reference)
			new_reference->addRef();
		if (reference)
			reference->release();
		reference = new_reference;
	}

	T* get() const noexcept
	{
		return reference;
	}

	T& operator*() const noexcept
	{
		return *reference;
	}

	T* operator->() const noexcept
	{
		return reference;
	}

	operator bool() const noexcept
	{
		return reference != nullptr;
	}

	friend bool operator==(const RHIRef&, const RHIRef&) noexcept = default;
	friend bool operator==(const RHIRef& lhs, std::nullptr_t) noexcept
	{
		return lhs.reference == nullptr;
	}
};

template <typename T, typename... Args>
RHIRef<T> makeRHIRef(Args&&... args)
{
	return RHIRef<T>::adopt(new T(std::forward<Args>(args)...));
}

}        // namespace Vortex
