
#include <cppalloc.hpp>
#include <cppalloc_export_debug.hxx>
#include <exception>
#include <iostream>
#include <vector>
#include <string_view>

struct dummy_tracer {
	struct backtrace {
		inline static std::size_t                                counter = 0;
		std::size_t               count   = counter++;
		template <typename stream> 
		friend stream& operator<<(stream& s, const backtrace& me) {
			s << "[INFO] trace_count = " << me.count;
			return s;
		}
		friend bool operator==(const backtrace& f, const backtrace& s) {
			return f.count == s.count;
		}
		friend bool operator!=(const backtrace& f, const backtrace& s) {
			return f.count != s.count;
		}

	};

	struct hasher {
		inline std::size_t operator()(const backtrace& t) const { return t.count; }
	};

	struct trace_output {
		inline void operator()(std::string_view s) const {
			std::cout << s << std::endl;
		}
	};
};

// verify empty class optim
struct empty_test_class_0 {
	float v[16];
};

struct empty_test_class_1 : public cppalloc::aligned_allocator<16> {
	float v[16];
};

using general_allocator =
    cppalloc::default_allocator<std::uint32_t, true, true, dummy_tracer>;
int main() {
	try {
		static_assert(sizeof(empty_test_class_0) == sizeof(empty_test_class_1),
		              "Sizes dont match");
		empty_test_class_1 datal;
		datal.v[4] = 0;
		std::cout << "[INFO] Size of 16 floats = " << sizeof(empty_test_class_1)
		                                        << std::endl;
		static_assert(cppalloc::traits::is_static_v<
		                  cppalloc::traits::tag_t<cppalloc::default_allocator<>>>,
		              "Default allocator must be static");
		static_assert(cppalloc::traits::is_static_v<
		                  cppalloc::traits::tag_t<general_allocator>>,
		              "Default allocator must be static");
		static_assert(cppalloc::traits::is_static_v<
		                  cppalloc::traits::tag_t<cppalloc::aligned_allocator<16>>>,
		              "Aligned allocator must be static");
		// General allocator test
		using alloc = cppalloc::default_allocator<std::size_t, true, true, dummy_tracer>;
		using alloc32 = cppalloc::aligned_allocator<32, std::size_t, true, true, dummy_tracer>;
		void* data = alloc::allocate(199);
		alloc::deallocate(data, 199);
		data = alloc::allocate(1991);
		alloc::deallocate(data, 1991);
		alloc::allocate(2199);
		for (std::uint64_t i = 0; i < 100; ++i)
			alloc32::deallocate(alloc32::allocate(i + 1024), i + 1024);
		for (std::uint64_t i = 0; i < 100; ++i)
			alloc::deallocate(alloc::allocate(i + 1024), i + 1024);
		
		// Force leak
		// alloc::deallocate(data, 2199);

		std::vector < std::uint32_t,
		            cppalloc::std_allocator_wrapper<std::uint32_t, cppalloc::default_allocator<>>>
		    vec_init = {3, 1, 12, 41};

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
