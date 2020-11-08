#include <catch2/catch.hpp>
#include <cppalloc.hpp>

struct alloc_mem_manager
{
  using arena_data_t = std::vector<std::uint8_t>;
  using alloc_info   = cppalloc::alloc_info<std::size_t>;
  struct allocation
  {
    alloc_info  info;
    std::size_t size  = 0;
    bool        freed = false;
  };
  std::vector<arena_data_t> arenas;
  std::vector<arena_data_t> backup_arenas;
  std::vector<allocation>   allocs;
  std::vector<allocation>   backup_allocs;

  bool drop_arena([[maybe_unused]] cppalloc::uhandle id)
  {
    arenas[id].clear();
    return true;
  }

  cppalloc::uhandle add_arena([[maybe_unused]] cppalloc::ihandle id, [[maybe_unused]] std::size_t size)
  {
    arena_data_t arena;
    arena.resize(size, 0xa);
    arenas.emplace_back(std::move(arena));
    return static_cast<cppalloc::uhandle>(arenas.size() - 1);
  }

  void begin_defragment()
  {
    backup_arenas = arenas;
    backup_allocs = allocs;
  }

  void end_defragment()
  {
    // assert validity
    for (std::size_t i = 0; i < allocs.size(); ++i)
    {
      auto& src = backup_allocs[i];
      auto& dst = allocs[i];

      assert(!std::memcmp(backup_arenas[src.info.harena].data() + src.info.offset,
                          backup_arenas[dst.info.harena].data() + dst.info.offset, src.size));
    }

    backup_arenas.clear();
    backup_arenas.shrink_to_fit();
    backup_allocs.clear();
    backup_allocs.shrink_to_fit();
  }

  void rebind_alloc([[maybe_unused]] cppalloc::uhandle halloc, alloc_info info)
  {
    assert(allocs[halloc].freed == false);
    allocs[halloc].info = info;
  }

  void move_memory([[maybe_unused]] cppalloc::uhandle src_arena, [[maybe_unused]] cppalloc::uhandle dst_arena,
                   [[maybe_unused]] std::size_t from, [[maybe_unused]] std::size_t to, std::size_t size)
  {
    assert(arenas[dst_arena].size() <= to + size);
    assert(arenas[src_arena].size() <= from + size);
    std::memcpy(arenas[dst_arena].data() + to, arenas[src_arena].data() + from, size);
  }
};

TEST_CASE("Validate arena_allocator", "[arena_allocator]") {}