
#include <cppalloc.hpp>
#include <exception>
#include <iostream>

int main() {
	try {

		cppalloc::best_fit_allocator<>::unit_test();
		cppalloc::best_fit_arena_allocator<
		    cppalloc::arena_manager_adapter>::unit_test();
		cppalloc::linear_allocator<>::unit_test();
		cppalloc::linear_arena_allocator<>::unit_test();
		cppalloc::pool_allocator<>::unit_test();
	} catch (std::exception ex) {
		std::cerr << "[ERROR] Failed with: " << ex.what();
		return -1;
	}
	std::cout << "[INFO] Tests OK.";
	return 0;
}
