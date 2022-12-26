# Single-header C++ VMT hooking (vfptr swap)

* Supports RAII.
* Unit tested with Catch2.
* Tested on x86/x64, MSVC and Clang/LLVM.
* VMT size calculation.
* Copies the RTTI object locator to ensure `dynamic_cast` won't break.

Usage:

```cpp
// Assumption: c_example looks like this
// 
// class c_example
// {
// public:
//     virtual ~c_example() = 0;
//     virtual void print() = 0;
// };

uintptr_t example_object = *reinterpret_cast<uintptr_t*>(0x0CA7F00D);
vfptr_swap_t swapper(example_object);

class c_hooked_example
{
public:
	void print(const char* text)
	{
		printf("c_example::print intercepted. og text: %s\n", text);
		Sleep(500);

		// *swapper returns the original object, although we could substitute it with example_object in this case
		swapper.original<void(__thiscall*)(uintptr_t, const char*)>(1)(*swapper, text);
	}	
};

void init()
{
	swapper[1] = vmt::cfunc(&c_hooked_example::print);
}
```
