cmake_minimum_required(VERSION 3.8)
project(catla)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

find_package(PythonLibs 3.10 REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(catla
        src/CatLa.cpp
        src/vm/modules/Type.cpp
        src/vm/modules/ClassField.cpp
        src/vm/modules/CodeBlock.cpp
        src/vm/modules/CatLaModule.cpp
        src/vm/modules/CatLaFunction.cpp
        src/vm/NyanVM.cpp
        src/util/Concurrent.h
        src/vm/stack/stack.cpp
        src/util/Benchmark.cpp
        src/heap/HeapAllocator.cpp
        src/vm/parser/VMParser.cpp
        src/vm/modules/Module.cpp)

llvm_map_components_to_libnames(llvm_libs
        Core
        ExecutionEngine
        Interpreter
        MC
        MCJIT
        Support
        nativecodegen)

target_include_directories(catla PRIVATE ${PYTHON_INCLUDE_DIRS})
target_link_libraries(catla ${llvm_libs} ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

if(WIN32)
    target_link_libraries(catla pthread_static_lib)
endif()