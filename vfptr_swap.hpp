﻿#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace vmt
{
	template <typename T>
	auto cfunc(T fn) -> uintptr_t
	{
		union UT { T fn; uintptr_t ptr; } u;
		u.fn = fn;

		return u.ptr;
	}

	auto calc_length(uintptr_t* vmt) -> std::size_t
	{
		auto is_valid_page = [](uintptr_t ptr) -> bool
		{
#ifdef _WIN32
			MEMORY_BASIC_INFORMATION mbi{};
			::VirtualQuery(reinterpret_cast<void*>(ptr), &mbi, sizeof(mbi));

			return
				(mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) > 0 && // executable
				(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) == 0; // no page protections block our access to read/execute
#else
			// Not 100% accurate. In POSIX, I have to manually parse /proc/self/maps and I'm not bothering.
			// It does not need to be 100% accurate though. We just need to copy *enough* virtual functions! More won't hurt.
			return ptr != 0 && *reinterpret_cast<uintptr_t**>(ptr) != nullptr;
#endif
		};

		std::size_t len = 0;

		while(is_valid_page(*vmt))
		{
			len++;
			vmt++;
		}

		return len;
	}
}

template <typename T>
class vfptr_swap_t
{
private:
	T* obj;
	uintptr_t* orig_vfptr;
	std::unique_ptr<uintptr_t[]> vmt;
	std::size_t vmt_size;

public:
	// vmt_size being set to 0 means that we're going to count the size of the methods in the VMT.
	// My VMT size calculation method is not heavily tested enough. Results might differ for certain VMT sizes due to alignment with 0s/the next VMT.
	// If the VMT's size is known, please specify it instead.
	vfptr_swap_t(T* _obj, std::size_t _vmt_size = 0) : obj(_obj), vmt_size(_vmt_size)
	{
		this->orig_vfptr = *reinterpret_cast<uintptr_t**>(obj);

		if(_vmt_size == 0)
		{
			this->vmt_size = vmt::calc_length(this->orig_vfptr);
		}

		// Accounting for RTTI.
		this->vmt = std::make_unique<uintptr_t[]>(this->vmt_size + 1);
		std::copy(this->orig_vfptr - 1, this->orig_vfptr + this->vmt_size, &this->vmt[0]);
		*reinterpret_cast<uintptr_t*>(this->obj) = reinterpret_cast<uintptr_t>(&this->vmt[1]);
	}

	// Restores original vfptr.
	~vfptr_swap_t()
	{
		*reinterpret_cast<uintptr_t*>(this->obj) = reinterpret_cast<uintptr_t>(this->orig_vfptr);
		this->vmt = nullptr;
	}

	// Returns the amount of virtual functions in the VMT.
	auto size() const -> std::size_t
	{
		return this->vmt_size;
	}

	// Returns a pointer to the beginning of the copied VMT.
	auto get_vmt() const -> uintptr_t*
	{
		return &this->vmt[1];
	}

	// Retrieves the RTTI object locator, if exists. Otherwise, returned data might be garbage.
	auto get_rtti() const -> uintptr_t*
	{
		return &this->get_vmt()[-1];
	}

	// Returns a virtual function by its index, from the copied VMT.
	// The returned virtual function will be hooked if it was replaced by the hooker, otherwise the original virtual function will be returned.
	auto operator[](std::size_t idx) const -> uintptr_t&
	{
		return this->get_vmt()[idx];
	}

	// Returns the object that is hooked.
	auto operator*() -> T*
	{
		return obj;
	}

	// Retrieves a specified function pointer from the original VMT.
	template <typename Y>
	auto original(std::size_t idx) const -> Y
	{
		return reinterpret_cast<Y>(this->orig_vfptr[idx]);
	}
};
