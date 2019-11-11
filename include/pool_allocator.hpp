#pragma once

#include <detail/cppalloc_common.hpp>
#include <default_allocator.hpp>

namespace cppalloc {

template <typename underlying_allocator =
              cppalloc::default_allocator<false>,
          bool k_compute_stats = false>
class pool_allocator : underlying_allocator, detail::statistics<k_compute_stats> {
public:

	using statistics = detail::statistics<k_compute_stats>;
	using size_type = typename underlying_allocator::size_type;
	using address   = typename underlying_allocator::address;

	template <typename...Args>
	pool_allocator(size_type i_atom_size, size_type i_atom_count, Args&&...i_args)
	    : k_atom_size(i_atom_size), k_atom_count(i_atom_count), underlying_allocator(std::forward<Args>(i_args)...) {}

	pool_allocator(pool_allocator&& i_other);
	
	inline address allocate(size_type i_size) { 

		size_type i_count = (i_size + k_atom_size - 1) / k_atom_size;
		if (i_count > k_atom_count)
			return underlying_allocator::allocate(i_size);

		auto measure = statistics::report_allocate(i_size);
		if (i_count == 1) {
			if (!solo) {
				if (!arrays)
					allocate_arena();
				return consume(1);
			}
			return consume();
		}		
		else {
			if (!arrays)
				allocate_arena();
			return consume(i_count);
		}		
	}

	inline void    deallocate(address i_ptr, size_type i_size) {
		size_type i_count = (i_size + k_atom_size - 1) / k_atom_size;
		if (i_count > k_atom_count)
			underlying_allocator::deallocate(i_ptr);
		auto measure = statistics::report_deallocate(i_size);
		if (i_count == 1)
			release(i_ptr);
		else
			release(i_ptr, i_count);
	}

private:
	struct array_arena {

		array_arena() : ppvalue(nullptr) {}
		array_arena(array_arena const&) = default;
		array_arena(array_arena&& other) : ppvalue(other.ppvalue) {
			other.ppvalue = nullptr;
		}
		array_arena(void* i_pdata) : pvalue(i_pdata) {}

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
			std::uintptr_t ivalue;
		};
	};

	struct solo_arena {
		solo_arena() : ppvalue(nullptr) {}
		solo_arena(solo_arena const&) = default;
		solo_arena(solo_arena&& other) : ppvalue(other.ppvalue) {
			other.ppvalue = nullptr;
		}
		solo_arena(void* i_pdata) : pvalue(i_pdata) {}
		solo_arena(array_arena const& other) : pvalue(other.get_value()) {}

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
		enum : size_type { k_header_size = sizeof(void*) };
		
		void link_with(array_arena arena, size_type size) {
			void** loc = reinterpret_cast<void**>(arena.value + size);
			*loc = first;
			first = loc;
		}

		template <typename lambda>
		void free_each(lambda&& i_deleter, size_t size) {
			size_type real_size = size + k_header_size;
			while (first) {
				void* next = *reinterpret_cast<void**>(first);
				i_deleter(reinterpret_cast<address>(reinterpret_cast<std::uintptr_t>(first) - size), real_size);
				first = next;
			}
			
		}

		void* first = nullptr;
	};

public:
	~pool_allocator() {
		size_type size = k_atom_count * k_atom_size;
		linked_arenas.free_each([=](address i_value, size_type i_size) {
			underlying_allocator::deallocate(i_value, i_size);
		}, size);
	}

	static void unit_test();
private:

	address consume(size_type i_count) {
		assert(arrays.length() >= i_count);
		std::uint8_t* ptr = arrays.get_value();
		std::uint8_t* head      = ptr + (i_count * k_atom_size);
		size_type left_over = arrays.length() - i_count;
		if (left_over == 1) {
			solo_arena solo ( head );
			arrays          = arrays.get_next();
			add_to_solo_queue(solo);
		} else {
			// reorder arrays to sort them from big to small
			array_arena save (head);
			save.set_length(left_over);
			array_arena cur  = arrays.get_next();
			if (cur.get_length() > left_over) {
				arrays           = cur;
				array_arena prev = save;
				while (true) {
					if (!cur || cur.get_length() <= left_over) {
						prev.set_next(save);
						save.set_next(cur);
						break;
					}
				}
			} else {
				save.set_next(cur);
				arrays = save;
			}

		}
		return ptr;
	}

	address consume() {
		std::uint8_t* ptr = solo.get_value();
		solo              = solo.get_next();
		return ptr;
	}

	void release(address i_only, size_type i_count) {
		array_arena new_arena(i_only, i_count);
		array_arena cur  = arrays;
			if (cur.get_length() > i_count) {
				array_arena prev = new_arena;
				while (true) {
					if (!cur || cur.get_length() <= i_count) {
						prev.set_next(new_arena);
						new_arena.set_next(cur);
						break;
					}
				}
			} else {
				new_arena.set_next(cur);
				arrays = new_arena;
			}
	}

	void release(address i_only) {
		solo_arena arena(i_only);
		arena.set_next(solo);
		solo = arena;
	}

	void allocate_arena() {
		size_type size = k_atom_count * k_atom_size;
		array_arena new_arena(underlying_allocator::allocate(size + arena_linker::k_header_size),
		                      k_atom_count);
		linked_arenas.link_with(new_arena, size);
		new_arena.set_next(arrays);
		arrays = new_arena;
		statistics::report_new_arena();
	}

	array_arena     arrays;
	solo_arena      solo;
	const size_type k_atom_count;
	const size_type k_atom_size;
	arena_linker linked_arenas;
};
} // namespace cppalloc