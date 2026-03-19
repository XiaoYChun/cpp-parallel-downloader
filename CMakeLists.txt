project(ParallelDownloader CXX)

# 查找库
find_package(CURL REQUIRED)

# 添加可执行文件
add_executable(ParallelDownloader curlgetexample.cpp)

# 链接目标文件和库
target_link_libraries(ParallelDownloader 
    CURL::libcurl
)  

set(CMAKE_CXX_STANDARD 17)