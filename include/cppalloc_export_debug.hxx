//! A single file must include this one time, this contains mostly debug info collectors
#pragma once
#include <cppalloc.hpp>

namespace cppalloc {
namespace detail {

#ifndef CPPALLOC_NO_STATS

detail::statistics<default_allocator_tag, true>
             default_allocator_statistics_instance;
detail::statistics<aligned_allocator_tag, true>
             aligned_alloc_statistics_instance;

#endif

template <typename size_type, typename stack_tracer, typename out_stream>
memory_tracker<size_type, stack_tracer, out_stream, true>& memory_tracker<
    size_type, stack_tracer, out_stream, true>::get_instance() {
	static memory_tracker<size_type, stack_tracer, out_stream, true> instance;
	return instance;
}
template <typename size_type, typename stack_tracer, typename out_stream>
memory_tracker<size_type, stack_tracer, out_stream, true>::~memory_tracker() {
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

template <typename size_type, typename stack_tracer, typename out_stream>
void memory_tracker<size_type, stack_tracer, out_stream, true>::when_allocate_(
    void* i_data, size_type i_size) {
	std::unique_lock<std::mutex> ul(lock);
	if (!ignore_first) {
		ignore_first = i_data;
		return;
	}
	pointer_map.emplace(
	    i_data,
	    std::make_pair(i_size, std::cref(regions.emplace(backtrace()).first)));
	memory_counter += i_size;
}
template <typename size_type, typename stack_tracer, typename out_stream>
void memory_tracker<size_type, stack_tracer, out_stream,
                    true>::when_deallocate_(void* i_data, size_type i_size) {
	std::unique_lock<std::mutex> ul(lock);
	if (i_data == ignore_first)
		return;
	auto                         it = pointer_map.find(i_data);
	if (it == pointer_map.end()) {
		out_stream::output("invalid memory free -> \n");
		out_stream::output(backtrace());
		throw;
	}
	pointer_map.erase(it);
	memory_counter -= i_size;
}
} // namespace detail
} // namespace cppalloc