
#include "best_fit_allocator.hpp"
#include "best_fit_arena_allocator.hpp"
#include "default_allocator.hpp"
#include "linear_allocator.hpp"
#include "linear_arena_allocator.hpp"
#include "pool_allocator.hpp"

namespace cppalloc {

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
}