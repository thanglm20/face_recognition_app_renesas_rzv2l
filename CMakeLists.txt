cmake_minimum_required(VERSION 3.8)
project(face_recognition_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TVM_ROOT "$ENV{TVM_HOME}" CACHE PATH "DRP-AI TVM root")
if(NOT TVM_ROOT)
    message(FATAL_ERROR "TVM_HOME must point to the TVM directory")
endif()

set(APP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(APP_SRC "${APP_ROOT}/src")
set(TVM_RUNTIME_LIB "${TVM_ROOT}/build_runtime/libtvm_runtime.so")

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS
    "${APP_SRC}/*.cpp"
    "${APP_SRC}/*.h"
)

add_executable(face_recognition_drpai_tvm_ws ${APP_SOURCES})

target_compile_definitions(face_recognition_drpai_tvm_ws PRIVATE
    FACE_RECOGNITION
)

target_include_directories(face_recognition_drpai_tvm_ws PRIVATE
    "${APP_SRC}"
    "${APP_SRC}/include"
    "${TVM_ROOT}/include"
    "${TVM_ROOT}/../apps"
    "${TVM_ROOT}/3rdparty/dlpack/include"
    "${TVM_ROOT}/3rdparty/dmlc-core/include"
    "${TVM_ROOT}/3rdparty/compiler-rt"
    "/usr/include"
)

find_package(OpenCV REQUIRED)
target_include_directories(face_recognition_drpai_tvm_ws PRIVATE ${OpenCV_INCLUDE_DIRS})

target_compile_options(face_recognition_drpai_tvm_ws PRIVATE
    -O3
    -DNDEBUG
    -Wall
)

target_link_directories(face_recognition_drpai_tvm_ws PRIVATE /usr/lib64)
target_link_libraries(face_recognition_drpai_tvm_ws PRIVATE
    pthread
    turbojpeg
    jpeg
    ${OpenCV_LIBS}
    ${TVM_RUNTIME_LIB}
)
