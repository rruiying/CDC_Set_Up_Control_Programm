set(UTILS_SOURCES
    src/logger.cpp
    src/config_manager.cpp
    src/math_utils.cpp
)

set(UTILS_HEADERS
    include/logger.h
    include/config_manager.h
    include/math_utils.h
)

add_library(utils_lib ${UTILS_SOURCES} ${UTILS_HEADERS})
target_include_directories(utils_lib PUBLIC include)
target_link_libraries(utils_lib
    Qt6::Core
)