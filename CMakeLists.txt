cmake_minimum_required(VERSION 3.8)
project(catla)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_executable(catla
        src/CatLa.cpp
        src/heap/HeapManager.cpp
        src/heap/HeapObject.cpp
        src/modules/CatLaClass.cpp
        src/modules/ClassField.cpp
        src/util/TypeUtil.hpp
        src/vm/CatVM.cpp
        )

llvm_map_components_to_libnames(llvm_libs
        Core
        ExecutionEngine
        Interpreter
        MC
        MCJIT
        Support
        nativecodegen)

target_link_libraries(catla ${llvm_libs})
