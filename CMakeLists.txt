cmake_minimum_required(VERSION 3.10)

set(target_name GSCamera)

project(${target_name} LANGUAGES CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 设置编译优化
#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
# 当函数返回值类型不为void但是函数内部没有任何返回时设置直接报错
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror=return-type")

message("start cmake.")

# 找 OpenCV
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
    message("this is aarch64 paltform.")
    include_directories(/usr/lib/aarch64-linux-gnu/glib-2.0/include)
    # 设置C编译器
    # set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include/gstreamer-1.0)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include/glib-2.0)
    #link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lib/)
elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
    message("this is x86_64 paltform.")
    include_directories(/usr/lib/x86_64-linux-gnu/glib-2.0/include/)
else()
    message("used aarch64 system platform:${CMAKE_HOST_SYSTEM_PROCESSOR}")
    include_directories(/usr/lib/aarch64-linux-gnu/glib-2.0/include)
    #include_directories(/usr/include/opencv4/)
    # 设置C编译器
    # set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include/gstreamer-1.0)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/include/glib-2.0)
    #link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lib/)
endif()

add_compile_options(-fPIC)

include_directories(/usr/include/gstreamer-1.0)
include_directories(/usr/include/glib-2.0/)

file(GLOB_RECURSE src
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(${target_name} SHARED "")
#add_library(${target_name} STATIC "")

target_sources(${target_name}
  PRIVATE
    ${src}
  )

# 添加主项目的包含目录
target_include_directories(${target_name}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
    target_link_libraries(${target_name}
    ${OpenCV_LIBS}
    -L${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lib/
    gstreamer-1.0
    gobject-2.0
    gmodule-2.0
    gthread-2.0
    glib-2.0
    gstvideo-1.0
    gstapp-1.0
    pthread
    png
    )
else()
    target_link_libraries(${target_name}
    ${OpenCV_LIBS}
    gstreamer-1.0
    gobject-2.0
    gmodule-2.0
    gthread-2.0
    glib-2.0
    gstvideo-1.0
    gstapp-1.0
    pthread
    png
    )
endif()



# install lib
install(
  TARGETS
    ${target_name}
  LIBRARY
    DESTINATION ${INSTALL_LIBDIR}
    COMPONENT lib
  ARCHIVE
    DESTINATION ${INSTALL_LIBDIR}
    COMPONENT lib
  )


