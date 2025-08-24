set(DATA_HEADERS
    include/data_processor.h
    include/export_manager.h
    include/file_manager.h
    include/csv_analyzer.h
)

set(DATA_SOURCES
    src/data_processor.cpp
    src/export_manager.cpp
    src/file_manager.cpp
    src/csv_analyzer.cpp
)

add_library(data_lib STATIC
    ${DATA_SOURCES}
    ${DATA_HEADERS}
)

target_include_directories(data_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(data_lib
    PUBLIC
        models_lib
        utils_lib
)

# 需要C++17 for std::filesystem
target_compile_features(data_lib PUBLIC cxx_std_17)