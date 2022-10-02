#pragma once

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
			// not 100% accurate. POSIX means i have to manually parse /proc/self/maps and i'm not botheri
			// it does not need to be 100% accurate though. we just need to copy *enough* virtual functions! more won't hurt
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
	uintptr_t* vmt;
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

		this->vmt = new uintptr_t[this->vmt_size];
		std::copy(this->orig_vfptr, this->orig_vfptr + this->vmt_size, &this->vmt[0]);
		*reinterpret_cast<uintptr_t*>(this->obj) = reinterpret_cast<uintptr_t>(this->vmt);
	}

	// Restores original vfptr
	~vfptr_swap_t()
	{
		*reinterpret_cast<uintptr_t*>(this->obj) = reinterpret_cast<uintptr_t>(this->orig_vfptr);
		delete[] this->vmt;
	}

	auto size() const -> std::size_t
	{
		return this->vmt_size;
	}

	auto get_vmt() const -> uintptr_t*
	{
		return this->vmt;
	}

	auto operator[](std::size_t idx) const -> uintptr_t&
	{
		return this->get_vmt()[idx];
	}

	auto operator*() -> T*
	{
		return obj;
	}

	template <typename T>
	auto original(std::size_t idx) const -> T
	{
		return reinterpret_cast<T>(this->orig_vfptr[idx]);
	}
};
