
#include "arena_allocator.hpp"
#include "default_allocator.hpp"
#include "linear_allocator.hpp"
#include "linear_arena_allocator.hpp"
#include "pool_allocator.hpp"
#include "std_allocator_wrapper.hpp"
#include "std_short_alloc.hpp"

namespace cppalloc
{

template <typename size_type = std::uint32_t>
constexpr size_type default_alignment = 0;

//! Define Allocator concept
//! template <typename T>
//! concept Allocator = requires(Allocator a) {
//! 		typename Allocator::size_type;
//! 		typename Allocator::address;
//! 		a.allocate(typename Allocator::size_type, Args&&...i_args)->typename
//! Allocator::address; 		a.deallocate(typename Allocator::address, typename
//! Allocator::size_type)->void;
//! }
//!
} // namespace cppalloc