add_executable(SpiteCompiler)
add_dependencies(SpiteCompiler SpiteRuntime)

set_target_properties(SpiteCompiler PROPERTIES
        OUTPUT_NAME spite
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON)

target_include_directories(SpiteCompiler PRIVATE ${SPITE_LLVM_INCLUDE_DIRS})
target_link_libraries(SpiteCompiler PRIVATE ${SPITE_LLVM_LIBS})
target_link_options(SpiteCompiler PRIVATE -static)

target_sources(SpiteCompiler
    PRIVATE
        main.cpp
)