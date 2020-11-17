#include <catch2/catch.hpp>
#include <cppalloc.hpp>

TEST_CASE("Validate linear_allocator", "[linear_allocator]")
{
  using namespace cppalloc;
  using allocator_t = linear_allocator<default_allocator<std::uint32_t, true>, true>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  constexpr std::uint32_t k_arena_size = 1000;
  allocator_t             allocator(k_arena_size);
  auto                    start  = cppalloc::allocate<std::uint8_t*>(allocator, 40);
  auto                    off100 = cppalloc::allocate<std::uint8_t*>(allocator, 100);
  CHECK(start + 40 == off100);
  allocator.deallocate(off100, 100);
  off100 = cppalloc::allocate<std::uint8_t*>(allocator, 100);
  CHECK(start + 40 == off100);
}

TEST_CASE("Validate linear_arena_allocator without alignment", "[linear_arena_allocator]")
{
  using namespace cppalloc;
  using allocator_t = linear_arena_allocator<default_allocator<std::uint32_t, 8, true>, true>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  constexpr std::uint32_t k_arena_size = 1000;
  allocator_t             allocator(k_arena_size);
  auto                    start  = cppalloc::allocate<std::uint8_t*>(allocator, 40);
  auto                    first  = start;
  auto                    off100 = cppalloc::allocate<std::uint8_t*>(allocator, 100);
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

TEST_CASE("Validate linear_arena_allocator with alignment", "[linear_arena_allocator]")
{
  using namespace cppalloc;
  using allocator_t = linear_arena_allocator<default_allocator<std::uint32_t, 128, true>, true>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  constexpr std::uint32_t k_arena_size = 1152;
  allocator_t             allocator(k_arena_size);
  auto                    start  = cppalloc::allocate<std::uint8_t*>(allocator, 256, 128);
  auto                    first  = start;
  auto                    off100 = cppalloc::allocate<std::uint8_t*>(allocator, 512, 128);
  CHECK(start + 256 == off100);
  allocator.deallocate(off100, 512);
  off100 = cppalloc::allocate<std::uint8_t*>(allocator, 512, 128);
  CHECK(start + 256 == off100);
  auto new_arena = cppalloc::allocate<std::uint8_t*>(allocator, 1024, 128);
  CHECK(2 == allocator.get_arena_count());
  auto from_old = cppalloc::allocate<std::uint8_t*>(allocator, 256);
  CHECK(off100 + 512 == from_old);
  allocator.deallocate(new_arena, 1024);
  new_arena = cppalloc::allocate<std::uint8_t*>(allocator, 1024, 128);
  CHECK(2 == allocator.get_arena_count());
  allocator.rewind();
  start = cppalloc::allocate<std::uint8_t*>(allocator, 64, 128);
  CHECK(start == first);
  CHECK(2 == allocator.get_arena_count());
  allocator.smart_rewind();
  start = cppalloc::allocate<std::uint8_t*>(allocator, 64, 128);
  CHECK(start == first);
  CHECK(1 == allocator.get_arena_count());
}

TEST_CASE("Validate linear_stack_allocator with alignment", "[linear_stack_allocator]")
{
  using namespace cppalloc;
  using allocator_t = linear_stack_allocator<default_allocator<std::uint32_t, 0, true>, true>;
  struct record
  {
    void*         data;
    std::uint32_t size;
  };
  constexpr std::uint32_t k_arena_size = 1;
  allocator_t             allocator(64);

  auto r1 = allocator.get_rewind_marker();
  auto a1  = cppalloc::allocate<std::uint8_t*>(allocator, 32, 0);
  allocator.rewind(r1);
  auto a2 = cppalloc::allocate<std::uint8_t*>(allocator, 32, 0);
  CHECK(a1 == a2);
  allocator.rewind(r1);
  a1  = cppalloc::allocate<std::uint8_t*>(allocator, 32, 0);
  a2  = cppalloc::allocate<std::uint8_t*>(allocator, 32, 0);
  auto a3  = cppalloc::allocate<std::uint8_t*>(allocator, 16, 0);
  auto r2 = allocator.get_rewind_marker();
  auto a4  = cppalloc::allocate<std::uint8_t*>(allocator, 32, 0);
  auto a6  = cppalloc::allocate<std::uint8_t*>(allocator, 32, 0);
  allocator.rewind(r2);
  auto a7  = cppalloc::allocate<std::uint8_t*>(allocator, 32, 0);
  CHECK(a4 == a7);
  auto r3 = allocator.get_rewind_marker();
  a7  = cppalloc::allocate<std::uint8_t*>(allocator, 2, 0);
  auto a8  = cppalloc::allocate<std::uint8_t*>(allocator, 128, 0);
  auto a9  = cppalloc::allocate<std::uint8_t*>(allocator, 32, 0);
  auto a10 = cppalloc::allocate<std::uint8_t*>(allocator, 64, 0);
  allocator.rewind(r3);
  auto a11 = cppalloc::allocate<std::uint8_t*>(allocator, 16, 0);
  CHECK(a7 == a11);
}
