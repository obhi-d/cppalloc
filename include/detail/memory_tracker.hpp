
#pragma once
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace cppalloc {
namespace detail {

template <typename size_type, typename stack_tracer, typename out_stream,
          bool enabled = false>
struct memory_tracker {
	inline static void* when_allocate(void* i_data, size_type i_size) {
		return i_data;
	}
	inline static void* when_deallocate(void* i_data, size_type i_size) {
		return i_data;
	}
};

template <typename size_type, typename stack_tracer, typename out_stream>
struct CPPALLOC_API memory_tracker<size_type, stack_tracer, out_stream, true> {

	using backtrace = typename stack_tracer;
	memory_tracker& get_instance();
	~memory_tracker();

	void when_allocate_(void* i_data, size_type i_size);
	void when_deallocate_(void* i_data, size_type i_size);

	static void* when_allocate(void* i_data, size_type i_size) {
		get_instance().when_allocate_(i_data, i_size);
		return i_data;
	}
	static void* when_deallocate(void* i_data, size_type i_size) {
		get_instance().when_deallocate_(i_data, i_size);
		return i_data;
	}

	std::unordered_map<
	    void*, std::pair<size_type, std::reference_wrapper<const backtrace>>>
	                              pointer_map;
	std::unordered_set<backtrace> regions;

	void*       ignore_first   = nullptr;
	std::size_t memory_counter = 0;
	std::mutex  lock;
};

} // namespace detail
} // namespace cppalloc