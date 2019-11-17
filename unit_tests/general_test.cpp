#define CPPALLOC_EXPORT_ENTRY
#include <cppalloc.hpp>
#include <exception>
#include <iostream>
#include <vector>

int main() {
	try {

		cppalloc::best_fit_allocator<>::unit_test();
		cppalloc::best_fit_arena_allocator<
		    cppalloc::arena_manager_adapter>::unit_test();
		cppalloc::linear_allocator<>::unit_test();
		cppalloc::linear_arena_allocator<>::unit_test();
		cppalloc::pool_allocator<>::unit_test();

		cppalloc::pool_allocator<> pool_allocator(8, 1000);

		using std_allocator = cppalloc::std_allocator_wrapper<std::uint64_t, cppalloc::pool_allocator<>>;
		std::vector<std::uint64_t, std_allocator> vlist =
		    std::vector<std::uint64_t, std_allocator>(std_allocator(pool_allocator));
		
		for (std::uint64_t i = 0; i < 1000; ++i)
			vlist.push_back(i);

	} catch (std::exception ex) {
		std::cerr << "[ERROR] Failed with: " << ex.what();
		return -1;
	}
	std::cout << "[INFO] Tests OK." << std::endl;
	return 0;
}
