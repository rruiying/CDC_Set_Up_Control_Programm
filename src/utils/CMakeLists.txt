set(UTILS_HEADERS
    include/logger.h
    include/math_utils.h
    include/statistics_utils.h
    include/string_utils.h
    include/time_utils.h
)

set(UTILS_SOURCES
    src/logger.cpp
    src/math_utils.cpp
    src/statistics_utils.cpp
    src/string_utils.cpp
    src/time_utils.cpp
)

add_library(utils_lib STATIC
    ${UTILS_SOURCES}
    ${UTILS_HEADERS}
)

target_include_directories(utils_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(utils_lib PUBLIC
    Threads::Threads
)

target_compile_features(utils_lib PUBLIC cxx_std_17)