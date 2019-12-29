#include <catch2/catch.hpp>
#include <cppalloc.hpp>

TEST_CASE("Validate pool_allocator", "[pool_allocator]") {
	using namespace cppalloc;
	using allocator_t =
	    pool_allocator<aligned_allocator<16, std::uint32_t, true>, true>;
	struct trivial_object {
		std::uint8_t value[16];
	};
	struct record {
		trivial_object* data;
		std::uint32_t   count;
	};
	constexpr std::uint32_t                      k_atom_count = 1000;
	std::vector<record>                          records;
	std::minstd_rand                             gen;
	std::bernoulli_distribution                  dice(0.6);
	std::uniform_int_distribution<std::uint32_t> generator(1, k_atom_count / 2);

	allocator_t allocator(sizeof(trivial_object), k_atom_count);

	auto validate = [&]() { return allocator.validate(records); };

	for (std::uint32_t allocs = 0; allocs < 10000; ++allocs) {
		if (dice(gen) || records.size() == 0) {
			record r;

			r.count = dice(gen) ? 1 : generator(gen);
			r.data  = reinterpret_cast<trivial_object*>(
          allocator.allocate(r.count * sizeof(trivial_object)));

			records.push_back(r);
			CHECK(validate());
		} else {
			std::uniform_int_distribution<std::uint32_t> choose(
			    0, static_cast<std::uint32_t>(records.size() - 1));
			std::uint32_t chosen = choose(gen);
			allocator.deallocate(records[chosen].data,
			                     records[chosen].count * sizeof(trivial_object));
			records.erase(records.begin() + chosen);
			CHECK(validate());
		}
	}
}

TEST_CASE("Validate std_allocator", "[std_allocator]") {

	using namespace cppalloc;
	cppalloc::pool_allocator<> pool_allocator(8, 1000);
	{
	using std_allocator =
	    cppalloc::std_allocator_wrapper<std::uint64_t,
	                                    cppalloc::pool_allocator<>>;
	std::vector<std::uint64_t, std_allocator> vlist =
	    std::vector<std::uint64_t, std_allocator>(std_allocator(pool_allocator));

	for (std::uint64_t i = 0; i < 1000; ++i)
		vlist.push_back(i);
	}
}