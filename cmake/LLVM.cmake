include(FetchContent)

FetchContent_Declare(llvmproject
    URL                 https://github.com/llvm/llvm-project/archive/refs/tags/llvmorg-23.1.3.tar.gz
    UPDATE_DISCONNECTED TRUE
    SOURCE_SUBDIR       llvm
    BINARY_DIR          "${CMAKE_BINARY_DIR}/_deps/llvmproject-build"
)

set(LLVM_ENABLE_PROJECTS    "lld" CACHE STRING "" FORCE)
set(LLVM_TARGETS_TO_BUILD   "X86" CACHE STRING "" FORCE)
set(LLVM_BUILD_TOOLS        OFF   CACHE BOOL   "" FORCE)
set(LLVM_BUILD_UTILS        OFF   CACHE BOOL   "" FORCE)
set(LLVM_ENABLE_ASSERTIONS  OFF   CACHE BOOL   "" FORCE)
set(LLVM_INCLUDE_TESTS      OFF   CACHE BOOL   "" FORCE)
set(LLVM_INCLUDE_EXAMPLES   OFF   CACHE BOOL   "" FORCE)
set(LLVM_INCLUDE_BENCHMARKS OFF   CACHE BOOL   "" FORCE)
set(LLVM_INCLUDE_DOCS       OFF   CACHE BOOL   "" FORCE)
set(LLVM_ENABLE_TERMINFO    OFF   CACHE BOOL   "" FORCE)
set(LLVM_ENABLE_ZLIB        OFF   CACHE BOOL   "" FORCE)
set(LLVM_ENABLE_ZSTD        OFF   CACHE BOOL   "" FORCE)
set(LLVM_ENABLE_LIBXML2     OFF   CACHE BOOL   "" FORCE)

function(_make_available_llvm)
    file(MAKE_DIRECTORY "${FETCHCONTENT_BASE_DIR}")
    file(LOCK "${FETCHCONTENT_BASE_DIR}/.populate.lock"
         GUARD FUNCTION
         TIMEOUT 1800
         RESULT_VARIABLE _spite_lock_rc)
    FetchContent_MakeAvailable(llvmproject)

    set(llvmproject_SOURCE_DIR "${llvmproject_SOURCE_DIR}" PARENT_SCOPE)
    set(llvmproject_BINARY_DIR "${llvmproject_BINARY_DIR}" PARENT_SCOPE)
endfunction()

_make_available_llvm()

set(SPITE_LLVM_LIBS
    LLVMCore LLVMTarget LLVMMC
    LLVMCodeGen LLVMX86CodeGen
    lldCommon lldMinGW lldCOFF)

set(SPITE_LLVM_INCLUDE_DIRS
    ${llvmproject_SOURCE_DIR}/llvm/include
    ${llvmproject_SOURCE_DIR}/lld/include
    ${llvmproject_BINARY_DIR}/include)
