add_library(
    Sequence INTERFACE
    sequence.hpp
    detail/container_compatible_range.hpp
    detail/exception_guard.hpp
    detail/range_guard.hpp
    detail/storage.hpp
    detail/traits.hpp
    detail/uninitialized.hpp
)
target_include_directories(
    Sequence INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)