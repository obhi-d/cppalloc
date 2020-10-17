//! A single file must include this one time, this contains mostly debug info collectors
#pragma once
#include <cppalloc.hpp>
#include <sstream>

namespace cppalloc
{
namespace detail
{

#ifndef CPPALLOC_NO_STATS

detail::statistics<default_allocator_tag, true> default_allocator_statistics_instance;

#endif

} // namespace detail
} // namespace cppalloc