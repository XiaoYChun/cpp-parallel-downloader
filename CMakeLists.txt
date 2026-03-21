cmake_minimum_required(VERSION 3.10)
project(ParallelDownloader CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(CURL REQUIRED)

set (EXAMPLE_FILE
    "example/curlgetexample.cpp"
    "example/singledownloader.cpp"
)
foreach (example_src_file ${EXAMPLE_FILE})
    # Set target name to example name
    # NAME_WE means name without extension(only file name)
    get_filename_component(target_name ${example_src_file} NAME_WE)

    # add executable file and link libcurl
    add_executable(${target_name} ${example_src_file})
    target_link_libraries(${target_name} PRIVATE CURL::libcurl)
endforeach()