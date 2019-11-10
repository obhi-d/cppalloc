
namespace cppalloc {

//! Define Allocator concept
//! template <typename T>
//! concept Allocator = requires(Allocator a) {
//! 		typename Allocator::size_type;
//! 		typename Allocator::address;
//! 		a.allocate(typename allocator::size_type, typename Allocator::size_type)->typename Allocator::address;
//! 		a.deallocate(typename Allocator::address, typename Allocator::size_type)->void;
//! }
//! 

}