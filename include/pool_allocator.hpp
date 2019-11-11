#pragma once

#include <default_allocator.hpp>
#include <detail/cppalloc_common.hpp>

namespace cppalloc {

struct pool_allocator_tag {};
template <typename underlying_allocator = cppalloc::default_allocator<>,
          bool k_compute_stats          = false>
class pool_allocator : detail::statistics<pool_allocator_tag, k_compute_stats,
                                          underlying_allocator> {
public:
	using statistics = detail::statistics<pool_allocator_tag, k_compute_stats,
	                                      underlying_allocator>;
	using size_type  = typename underlying_allocator::size_type;
	using address    = typename underlying_allocator::address;

	template <typename... Args>
	pool_allocator(size_type i_atom_size, size_type i_atom_count,
	               Args&&... i_args)
	    : k_atom_size(i_atom_size), k_atom_count(i_atom_count),
	      statistics(std::forward<Args>(i_args)...) {}

	pool_allocator(pool_allocator const& i_other) = delete;
	pool_allocator(pool_allocator&& i_other)      = default;

	inline address allocate(size_type i_size) {

		size_type i_count = (i_size + k_atom_size - 1) / k_atom_size;
		if (i_count > k_atom_count)
			return underlying_allocator::allocate(i_size);

		auto measure = statistics::report_allocate(i_size);
		if (i_count == 1) {
			if (!solo) {
				return consume(1);
			}
			return consume();
		} else {
			return consume(i_count);
		}
	}

	inline void deallocate(address i_ptr, size_type i_size) {
		size_type i_count = (i_size + k_atom_size - 1) / k_atom_size;
		if (i_count > k_atom_count)
			underlying_allocator::deallocate(i_ptr, i_size);
		auto measure = statistics::report_deallocate(i_size);
		if (i_count == 1)
			release(i_ptr);
		else
			release(i_ptr, i_count);
	}

private:
	struct array_arena {

		array_arena() : ppvalue(nullptr) {}
		array_arena(array_arena const& other) : ppvalue(other.ppvalue) {}
		array_arena(array_arena&& other) : ppvalue(other.ppvalue) {
			other.ppvalue = nullptr;
		}
		array_arena(void* i_pdata) : pvalue(i_pdata) {}
		array_arena& operator=(array_arena&& other) {
			ppvalue       = other.ppvalue;
			other.ppvalue = nullptr;
			return *this;
		}
		array_arena& operator=(array_arena const& other) {
			pvalue = other.pvalue;
			return *this;
		}
		explicit array_arena(address i_addr, size_type i_count)
		    : ivalue(reinterpret_cast<std::uintptr_t>(i_addr) | 0x1) {
			*reinterpret_cast<size_type*>(i_addr) = i_count;
		}

		size_type length() const {
			return *reinterpret_cast<size_type*>(ivalue & ~0x1);
		}

		void set_length(size_type i_length) {
			*reinterpret_cast<size_type*>(ivalue & ~0x1) = i_length;
		}

		array_arena get_next() const {
			return *(reinterpret_cast<void**>(ivalue & ~0x1) + 1);
		}

		void set_next(array_arena next) {
			*(reinterpret_cast<void**>(ivalue & ~0x1) + 1) = next.value;
		}

		void clear_flag() { ivalue &= (~0x1); }

		std::uint8_t* get_value() const {
			return reinterpret_cast<std::uint8_t*>(ivalue & ~0x1);
		}
		void* update(size_type i_count) { return get_value() + i_count; }

		operator bool() const { return ppvalue != nullptr; }
		union {
			address        addr;
			void*          pvalue;
			void**         ppvalue;
			std::uint8_t*  value;
			std::uintptr_t ivalue;
		};
	};

	struct solo_arena {
		solo_arena() : ppvalue(nullptr) {}
		solo_arena(solo_arena const&& other) : ppvalue(other.ppvalue) {}
		solo_arena(void* i_pdata) : pvalue(i_pdata) {}
		solo_arena(solo_arena&& other) : ppvalue(other.ppvalue) {
			other.ppvalue = nullptr;
		}
		solo_arena(solo_arena const& other) : pvalue(other.pvalue) {}
		solo_arena(array_arena const& other) : pvalue(other.get_value()) {}
		solo_arena& operator=(solo_arena&& other) {
			ppvalue       = other.ppvalue;
			other.ppvalue = nullptr;
			return *this;
		}
		solo_arena& operator=(array_arena const& other) {
			pvalue = other.get_value();
			return *this;
		}
		solo_arena& operator=(solo_arena const& other) {
			pvalue = other.pvalue;
			return *this;
		}

		void* get_value() const { return pvalue; }

		solo_arena get_next() const { return *(ppvalue); }
		void       set_next(solo_arena next) { *(ppvalue) = next.value; }

		operator bool() const { return ppvalue != nullptr; }
		union {
			address        addr;
			void*          pvalue;
			void**         ppvalue;
			std::uint8_t*  value;
			std::uintptr_t ivalue;
		};
	};

	struct arena_linker {
		arena_linker()                                     = default;
		explicit arena_linker(const arena_linker& i_other) = default;
		arena_linker(arena_linker&& i_other) : first(i_other.first) {
			i_other.first = nullptr;
		}

		enum : size_type { k_header_size = sizeof(void*) };

		void link_with(address arena, size_type size) {
			void** loc =
			    reinterpret_cast<void**>(static_cast<std::uint8_t*>(arena) + size);
			*loc  = first;
			first = loc;
		}

		template <typename lambda>
		void for_each(lambda&& i_deleter, size_type size) {
			size_type real_size = size + k_header_size;
			void*     it        = first;
			while (it) {
				void* next = *reinterpret_cast<void**>(it);
				i_deleter(reinterpret_cast<address>(
				              reinterpret_cast<std::uint8_t*>(it) - size),
				          real_size);
				it = next;
			}
		}

		      operator bool() const { return first != nullptr; }
		void* first = nullptr;
	};

public:
	~pool_allocator() {
		size_type size = k_atom_count * k_atom_size;
		linked_arenas.for_each(
		    [=](address i_value, size_type i_size) {
			    underlying_allocator::deallocate(i_value, i_size);
		    },
		    size);
	}

private:
	address consume(size_type i_count) {
		size_type len;
		if (!arrays || (len = arrays.length()) < i_count) {
			allocate_arena();
			len = arrays.length();
		}

		assert(len >= i_count);
		std::uint8_t* ptr       = arrays.get_value();
		std::uint8_t* head      = ptr + (i_count * k_atom_size);
		size_type     left_over = len - i_count;
		switch (left_over) {
		case 0:
			arrays = arrays.get_next();
			break;
		case 1: {
			solo_arena new_solo(head);
			arrays = arrays.get_next();
			new_solo.set_next(solo);
			solo = new_solo;
		} break;
		default: {
			// reorder arrays to sort them from big to small
			array_arena cur = arrays.get_next();
			array_arena save(head, left_over);
			if (cur && cur.length() > left_over) {
				arrays           = cur;
				array_arena prev = save;
				while (true) {
					if (!cur || cur.length() <= left_over) {
						prev.set_next(save);
						save.set_next(cur);
						break;
					}
					prev = cur;
					cur  = cur.get_next();
				}
			} else {
				save.set_next(cur);
				arrays = save;
			}
		} break;
		}

		return ptr;
	}

	address consume() {
		address ptr = solo.get_value();
		solo        = solo.get_next();
		return ptr;
	}

	void release(address i_only, size_type i_count) {
		array_arena new_arena(i_only, i_count);
		array_arena cur = arrays;
		if (cur.length() > i_count) {
			array_arena prev = new_arena;
			while (true) {
				if (!cur || cur.length() <= i_count) {
					prev.set_next(new_arena);
					new_arena.set_next(cur);
					break;
				}
				prev = cur;
				cur  = cur.get_next();
			}
		} else {
			new_arena.set_next(cur);
			arrays = new_arena;
		}
	}

	void release(address i_only) {
		solo_arena arena(i_only);
		arena.set_next(std::move(solo));
		solo = arena;
	}

	void allocate_arena() {
		size_type size = k_atom_count * k_atom_size;
		address   arena_data =
		    underlying_allocator::allocate(size + arena_linker::k_header_size);
		array_arena new_arena(arena_data, k_atom_count);
		linked_arenas.link_with(arena_data, size);
		new_arena.set_next(arrays);
		arrays = new_arena;
		statistics::report_new_arena();
	}

	std::uint32_t get_total_free_count() const {
		std::uint32_t count   = 0;
		auto          a_first = arrays;
		while (a_first) {
			count += a_first.length();
			a_first = a_first.get_next();
		}

		auto s_first = solo;
		while (s_first) {
			count++;
			s_first = s_first.get_next();
		}
		return count;
	}

	std::uint32_t get_total_arena_count() const {
		std::uint32_t count = 0;
		arena_linker  a_first(linked_arenas);
		a_first.for_each([&](address i_value, size_type i_size) { count++; },
		                 k_atom_size * k_atom_count);
		return count;
	}

	array_arena     arrays;
	solo_arena      solo;
	const size_type k_atom_count;
	const size_type k_atom_size;
	arena_linker    linked_arenas;

public:
	template <typename record_ty> bool validate(record_ty const& records) {
		std::uint32_t rec_count = 0;
		for (auto& rec : records)
			rec_count += rec.count;

		std::uint32_t arena_count = get_total_arena_count();
		if (rec_count + get_total_free_count() != arena_count * k_atom_count)
			return false;

		if (arena_count != this->statistics::get_arenas_allocated())
			return false;
		return true;
	}

	static void unit_test() {
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
				assert(validate());
			} else {
				std::uniform_int_distribution<std::uint32_t> choose(
				    0, static_cast<std::uint32_t>(records.size() - 1));
				std::uint32_t chosen = choose(gen);
				allocator.deallocate(records[chosen].data,
				                     records[chosen].count * sizeof(trivial_object));
				records.erase(records.begin() + chosen);
				assert(validate());
			}
		}
	}
};
} // namespace cppalloc