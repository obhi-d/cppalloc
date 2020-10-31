#include <catch2/catch.hpp>
#include <cppalloc.hpp>

TEST_CASE("Validate best_fit_allocator", "[best_fit_allocator]")
{
  cppalloc::best_fit_allocator<>               allocator;
  std::minstd_rand                             gen;
  std::bernoulli_distribution                  dice(0.7);
  std::uniform_int_distribution<std::uint32_t> generator(1, 100000);

  struct record
  {
    std::uint32_t offset, size;
  };
  std::vector<record> allocated;
  for (std::uint32_t allocs = 0; allocs < 10000; ++allocs)
  {
    if (dice(gen) || allocated.size() == 0)
    {
      record r;
      r.size   = generator(gen);
      r.offset = allocator.allocate(r.size);
      if (r.offset != cppalloc::best_fit_allocator<>::k_invalid_offset)
        allocated.push_back(r);
      CHECK(allocator.validate() == true);
    }
    else
    {
      std::uniform_int_distribution<std::uint32_t> choose(0, static_cast<std::uint32_t>(allocated.size() - 1));
      std::uint32_t                                chosen = choose(gen);
      allocator.deallocate(allocated[chosen].offset, allocated[chosen].size);
      allocated.erase(allocated.begin() + chosen);
      CHECK(allocator.validate() == true);
    }
  }
}

TEST_CASE("Validate best_fit_arena_allocator", "[best_fit_arena_allocator]")
{
  using namespace cppalloc;
  using address_t = typename detail::bf_address_type<std::uint32_t>;
  struct record
  {
    address_t     offset;
    std::uint32_t size;
    std::uint32_t alignment;
  };

  struct example_manager : arena_manager_adapter
  {
    std::vector<record>        allocated;
    std::vector<std::uint32_t> valids;
    std::vector<std::uint32_t> invalids;

    using allocator_t = best_fit_arena_allocator<example_manager, std::uint32_t, true>;

    void move_memory([[maybe_unused]] address src, [[maybe_unused]] address dst, [[maybe_unused]] std::uint32_t size,
                     [[maybe_unused]] typename allocator_t::move_iterator iterator)
    {
      std::uint32_t index;
      while (iterator.has_next(index))
      {
        record& rec = allocated[index];
        iterator.modify_offset(rec.offset.offset);
        assert((rec.offset.offset & (rec.alignment - 1)) == 0);
      }
    }
  };
  example_manager manager;
  using allocator_t = best_fit_arena_allocator<example_manager, std::uint32_t, true>;
  allocator_t                                  allocator(150000, manager);
  std::minstd_rand                             gen;
  std::bernoulli_distribution                  dice(0.7);
  std::uniform_int_distribution<std::uint32_t> generator(1, 10000);
  std::uniform_int_distribution<std::uint32_t> generator2(1, 10);

  std::vector<record>& allocated = manager.allocated;
  for (std::uint32_t allocs = 0; allocs < 10000; ++allocs)
  {
    if (dice(gen) || manager.valids.size() == 0)
    {
      record        r;
      std::uint32_t handle = 0;

      if (manager.invalids.size() > 0)
      {
        handle = manager.invalids.back();
        manager.invalids.pop_back();
      }
      else
      {
        handle = static_cast<std::uint32_t>(allocated.size());
        allocated.resize(static_cast<std::vector<record, std::allocator<record>>::size_type>(handle) + 1);
      }

      r.size      = generator(gen);
      r.alignment = 1 << generator2(gen);
      r.offset    = allocator.allocate(r.size, r.alignment, handle, dice(gen) ? allocator_t::f_defrag : 0);
      CHECK((r.offset.offset & (r.alignment - 1)) == 0);
      allocated[handle] = r;
      manager.valids.push_back(handle);
      assert(allocator.validate());
      CHECK(allocator.validate());
    }
    else
    {
      std::uniform_int_distribution<std::uint32_t> choose(0, static_cast<std::uint32_t>(manager.valids.size() - 1));
      std::uint32_t                                chosen = choose(gen);
      std::uint32_t                                handle = manager.valids[chosen];
      auto&                                        rec    = allocated[handle];
      allocator.deallocate(rec.offset, rec.size, rec.alignment);
      manager.valids.erase(manager.valids.begin() + chosen);
      manager.invalids.push_back(handle);
      CHECK(allocator.validate());
    }
  }
}