# Single-header C++ VMT hooking (vfptr swap)

* Supports RAII.
* Unit tested with Catch2.
* Tested on x86/x64, MSVC and Clang/LLVM.
* VMT size calculation.
* Copies the RTTI object locator to ensure `dynamic_cast` won't break'.

Usage: (demonstrated on MapleStory 8.83.0.1)

```cpp
// Assumption: CUser looks like this
// 
// class CUser
// {
// public:
//     virtual ~CUser() = 0;
//     virtual void Update() = 0;
// };

uintptr_t local_player = *reinterpret_cast<uintptr_t*>(0x00BEBF98);
vfptr_swap_t swapper(local_player);

class hooked_user
{
public:
	void update()
	{
		printf("cuser::update intercepted\n");
		Sleep(500);

		// *swapper returns the original object, although we could substitute it with local_player in this case
		swapper.original<void(__thiscall*)(uintptr_t ecx)>(1)(*swapper);
	}	
};

void init()
{
	swapper[1] = vmt::cfunc(&hooked_user::update);
}
```
