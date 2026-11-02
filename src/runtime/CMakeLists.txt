add_library(SpiteRuntime STATIC)

set_target_properties(SpiteRuntime PROPERTIES
        OUTPUT_NAME spite_rt
        C_STANDARD 11
        C_STANDARD_REQUIRED ON)

target_compile_options(SpiteRuntime PRIVATE -ffreestanding)

target_sources(SpiteRuntime
    PRIVATE
        runtime.c
)