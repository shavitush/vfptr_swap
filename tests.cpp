#include "vfptr_swap.hpp"

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

class parent_t
{
private:
	int padding[0x10];

public:
	virtual ~parent_t() = default;

	virtual auto generate_random_number(int low_bound = 0, int high_bound = 100) -> int
	{
		return (std::rand() % (high_bound - low_bound + 1)) + low_bound;
	}

	virtual auto is_padding_size_correct() const -> bool
	{
		return sizeof(padding) / sizeof(padding[0]) == 0x10;
	}

	virtual auto always_returns_true() const -> bool
	{
		return true;
	}
};

class child_t : public parent_t
{
public:
	virtual int generate_random_number(int low_bound = 0, int high_bound = 100)
	{
		throw std::logic_error("unimplemented");
	}

	// pad an extra function to test ::size()
	virtual void empty() {}
};

vfptr_swap_t<child_t>* g_swapper = nullptr;

class hooked_child_t
{
public:
	auto get_rng(int a1, int a2) -> int
	{
		return 1337;
	}

	auto ret_false_classmember() -> bool
	{
		printf("ret_false_classmember | orig returned %d\n", g_swapper->original<bool(__thiscall*)(child_t* ecx)>(2)(**g_swapper));

		return false;
	}
};

bool __fastcall ret_false_static(uintptr_t ecx, uintptr_t)
{
	return false;
}

TEST_CASE()
{
	child_t child;
	auto _child = &child; // if we call functions as `child.func()` then the compiler won't generate a virtual call but rather call the class member function, passing (ecx, args...) to it
	g_swapper = new vfptr_swap_t<child_t>(_child); // global ptr so we can use it to call the original through the hooked functions
	REQUIRE(g_swapper->size() == 5);

	(*g_swapper)[1] = vmt::cfunc(&hooked_child_t::get_rng);
	REQUIRE(_child->generate_random_number() == 1337);

	(*g_swapper)[2] = vmt::cfunc(&hooked_child_t::ret_false_classmember);
	REQUIRE(!_child->is_padding_size_correct());

	delete g_swapper; // vfptr_swap_t dtor is called, no hooks are applied

	// func should throw an "unimplemented" exception when unhooked
	bool thrown = false;
	try { _child->generate_random_number(); }
	catch(const std::logic_error&) { thrown = true; }

	REQUIRE(thrown);
	REQUIRE(_child->is_padding_size_correct());

	// RAII is supported as well
	parent_t parent;
	auto _parent = &parent;
	{
		vfptr_swap_t swapper(_parent);
		REQUIRE(swapper.size() == 4);
		swapper[3] = vmt::cfunc(ret_false_static);
		REQUIRE(!_parent->always_returns_true());
	}

	REQUIRE(_parent->always_returns_true());
}
