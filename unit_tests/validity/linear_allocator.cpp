#include <catch2/catch.hpp>
#include <cppalloc.hpp>

TEST_CASE("Validate linear_allocator", "[linear_allocator]") {
	using namespace cppalloc;
	using allocator_t =
	    linear_allocator<default_allocator<std::uint32_t, true>, true>;
	struct record {
		void*         data;
		std::uint32_t size;
	};
	constexpr std::uint32_t k_arena_size = 1000;
	allocator_t             allocator(k_arena_size);
	auto start  = cppalloc::allocate<std::uint8_t*>(allocator, 40);
	auto off100 = cppalloc::allocate<std::uint8_t*>(allocator, 100);
	CHECK(start + 40 == off100);
	allocator.deallocate(off100, 100);
	off100 = cppalloc::allocate<std::uint8_t*>(allocator, 100);
	CHECK(start + 40 == off100);
}

TEST_CASE("Validate linear_arena_allocator", "[linear_arena_allocator]") {
	using namespace cppalloc;
	using allocator_t =
	    linear_arena_allocator<default_allocator<std::uint32_t, true>, true>;
	struct record {
		void*         data;
		std::uint32_t size;
	};
	constexpr std::uint32_t k_arena_size = 1000;
	allocator_t             allocator(k_arena_size);
	auto start  = cppalloc::allocate<std::uint8_t*>(allocator, 40);
	auto first  = start;
	auto off100 = cppalloc::allocate<std::uint8_t*>(allocator, 100);
	CHECK(start + 40 == off100);
	allocator.deallocate(off100, 100);
	off100 = cppalloc::allocate<std::uint8_t*>(allocator, 100);
	CHECK(start + 40 == off100);
	auto new_arena = cppalloc::allocate<std::uint8_t*>(allocator, 1000);
	CHECK(2 == allocator.get_arena_count());
	auto from_old = cppalloc::allocate<std::uint8_t*>(allocator, 40);
	CHECK(off100 + 100 == from_old);
	allocator.deallocate(new_arena, 1000);
	new_arena = cppalloc::allocate<std::uint8_t*>(allocator, 1000);
	CHECK(2 == allocator.get_arena_count());
	allocator.rewind();
	start = cppalloc::allocate<std::uint8_t*>(allocator, 40);
	CHECK(start == first);
	CHECK(2 == allocator.get_arena_count());
	allocator.smart_rewind();
	start = cppalloc::allocate<std::uint8_t*>(allocator, 40);
	CHECK(start == first);
	CHECK(1 == allocator.get_arena_count());
}
