#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <cppalloc_export_debug.hxx>

TEST_CASE("Validate general_allocator", "[general_allocator]")
{

  using namespace cppalloc;
  using allocator_t = default_allocator<std::uint32_t, true, false>;

  allocator_t::address data = allocator_t::allocate(32, 1000);
  allocator_t::deallocate(data, 32, 1000);
}