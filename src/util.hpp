#ifndef UTIL_HPP
#define UTIL_HPP

#include <type_traits>
#include <concepts>

template <typename T>
concept IsVectorOrArray = requires(T t) {
    typename T::value_type; // Check if it has a value_type member
    typename T::size_type;  // Check if it has a size_type member
    t.size();               // Check if it has a size() member function
    t.data();               // Check if it has a data() member function
    //sizeof( typename T::value_type) == 4; // Check that only containers with 4 bytes size are allowed.
    //std::span<const typename T::value_type>(t); // Check if the type can be converted to std::span
};

#endif