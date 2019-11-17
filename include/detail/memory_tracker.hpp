
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
#ifdef CPPALLOC_EXPORT_ENTRY
	memory_tracker& get_instance() { 
		static memory_tracker instance;
		return instance;
	}
	~memory_tracker() {
		if (pointer_map.size() > 0) {
			out_stream::output("Possible leaks\n");
			std::ostringstream stream;
			for (auto& e : pointer_map) {
				stream << "\n\t [" << e.first << "] for " << e.second.first
				       << " bytes from\n"
						 << e.second.second;
			}
			out_stream::output(stream.str());
		}
	}

	void when_allocate_(void* i_data, size_type i_size) {
		std::unique_lock<std::mutex> ul(lock);
		pointer_map.emplace(
		    i_data,
		    std::make_pair(i_size, std::cref(regions.emplace(backtrace()).first)));
		memory_counter += i_size;
	}

	void when_deallocate_(void* i_data, size_type i_size) {
		std::unique_lock<std::mutex> ul(lock);
		auto                         it = pointer_map.find(i_data);
		if (it == pointer_map.end()) {
			out_stream::output("invalid memory free -> \n");
			out_stream::output(backtrace());
			throw;
		}
		pointer_map.erase(it);
	}
#else
	memory_tracker& get_instance();
	~memory_tracker();

	void when_allocate_(void* i_data, size_type i_size);
	void when_deallocate_(void* i_data, size_type i_size);

#endif
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

	std::size_t memory_counter = 0;
	std::mutex  lock;
};

} // namespace detail
} // namespace cppalloc
