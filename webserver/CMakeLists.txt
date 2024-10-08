# 参考的简化版本 https://github.com/Liluoquan/a-high-concurrency-webserver-in-linux
project(WebServer LANGUAGES CXX)
cmake_minimum_required(VERSION 3.5.1)

#项目根目录
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
#源码目录
set(SRC_DIR ${ROOT_DIR}/src)
#头文件路径
set(INC_DIR ${ROOT_DIR}/include)
#库文件路径
set(LIB_DIR /usr/lib/x86_64-linux-gnu)
#可执行文件安装路径
set(EXEC_INSTALL_DIR ${ROOT_DIR}/bin)
#库安装路径
set(LIB_INSTALL_DIR ${ROOT_DIR}/lib)

#g++ 编译选项 -Wl,-rpath指定运行时库的搜索路径 
#使用-Wl,-Bstatic后面的库都以静态链接，-Wl,-Bdynamic后面的库都以动态链接 
#还可以使用-static来避免动态链接, 此方法会导致对所有的库都以静态链接
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wfatal-errors -Wno-unused-parameter -Wl,-rpath=${LIB_INSTALL_DIR}")

#CPP源文件
file(GLOB UTILS_SRC_FILE          ${SRC_DIR}/utils/*.cc)
file(GLOB THREAD_SRC_FILE         ${SRC_DIR}/thread/*.cc)
file(GLOB LOG_SRC_FILE            ${SRC_DIR}/log/*.cc)
file(GLOB MEMORY_SRC_FILE         ${SRC_DIR}/memory/*.cc)
file(GLOB CACHE_SRC_FILE          ${SRC_DIR}/cache/*.cc)
file(GLOB EVENT_SRC_FILE          ${SRC_DIR}/event/*.cc)
file(GLOB NET_SRC_FILE            ${SRC_DIR}/net/*.cc)
file(GLOB HTTP_SRC_FILE           ${SRC_DIR}/http/*.cc)
set(MAIN_SRC_FILE                 ${ROOT_DIR}/main.cc)

set(LIBEVENT_SOURCES ${UTILS_SRC_FILE}
                     ${THREAD_SRC_FILE}
                     ${LOG_SRC_FILE}
                     ${MEMORY_SRC_FILE}
                     ${CACHE_SRC_FILE}
                     ${EVENT_SRC_FILE}
                     ${NET_SRC_FILE}
                     ${HTTP_SRC_FILE})

#头文件路径添加
include_directories(${INC_DIR})
#库文件路径添加
link_directories(${LIB_DIR})
#用到的库
set(LINK_LIBRARY pthread)

#生成动态链接库
add_library(event_shared SHARED ${LIBEVENT_SOURCES})
set_target_properties(event_shared PROPERTIES OUTPUT_NAME event)
target_link_libraries(event_shared ${LINK_LIBRARY})

#生成静态库
add_library(event_static STATIC ${LIBEVENT_SOURCES})
set_target_properties(event_static PROPERTIES OUTPUT_NAME event)
target_link_libraries(event_static ${LINK_LIBRARY})

#生成可执行文件
add_executable(webserver ${MAIN_SRC_FILE})
target_link_libraries(webserver event_static ${LINK_LIBRARY})

#安装
install(TARGETS webserver DESTINATION ${EXEC_INSTALL_DIR})
install(TARGETS event_shared DESTINATION ${LIB_INSTALL_DIR})
install(TARGETS event_static DESTINATION ${LIB_INSTALL_DIR})
