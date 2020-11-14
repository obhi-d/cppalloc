#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <cppalloc_export_debug.hxx>

TEST_CASE("Validate general_allocator", "[general_allocator]")
{

  using namespace cppalloc;
  using allocator_t = default_allocator<std::uint32_t, 0, true, false>;

  allocator_t::address data = allocator_t::allocate(256, 128);
  CHECK((reinterpret_cast<std::uintptr_t>(data) & 127) == 0);
  allocator_t::deallocate(data, 256, 128);
}