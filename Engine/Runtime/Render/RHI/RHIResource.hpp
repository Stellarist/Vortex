#pragma once

#include <atomic>
#include <cassert>
#include <type_traits>
#include <utility>

class RHIResource {
private:
	mutable std::atomic_uint32_t ref_count{1};

protected:
	RHIResource() = default;
	virtual ~RHIResource() noexcept = default;

public:
	RHIResource(const RHIResource&) = delete;
	RHIResource& operator=(const RHIResource&) = delete;
	RHIResource(RHIResource&&) = delete;
	RHIResource& operator=(RHIResource&&) = delete;

	uint32_t addRef() const noexcept
	{
		return ref_count.fetch_add(1, std::memory_order_relaxed) + 1;
	}

	uint32_t release() const noexcept
	{
		const uint32_t previous_count = ref_count.fetch_sub(1, std::memory_order_acq_rel);
		assert(previous_count > 0 && "Cannot release an RHI resource with no references.");
		if (previous_count == 1)
			delete this;
		return previous_count - 1;
	}

	uint32_t getRefCount() const noexcept
	{
		return ref_count.load(std::memory_order_relaxed);
	}
};


template <typename T>
class RHIRef {
private:
	T* reference{};

	struct AdoptTag {};

	explicit RHIRef(T* new_reference, AdoptTag) noexcept :
	    reference(new_reference)
	{}

	template <typename U>
	friend class RHIRef;

public:
	RHIRef() noexcept = default;
	RHIRef(std::nullptr_t) noexcept
	{}

	explicit RHIRef(T* new_reference) noexcept :
	    reference(new_reference)
	{
		if (reference)
			reference->addRef();
	}

	RHIRef(const RHIRef& other) noexcept :
	    RHIRef(other.reference)
	{}

	template <typename U>
	    requires std::is_convertible_v<U*, T*>
	RHIRef(const RHIRef<U>& other) noexcept :
	    RHIRef(other.reference)
	{}

	RHIRef(RHIRef&& other) noexcept :
	    reference(std::exchange(other.reference, nullptr))
	{}

	template <typename U>
	    requires std::is_convertible_v<U*, T*>
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

	template <typename U>
	    requires std::is_convertible_v<U*, T*>
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

	template <typename U>
	    requires std::is_convertible_v<U*, T*>
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
	explicit operator bool() const noexcept
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
