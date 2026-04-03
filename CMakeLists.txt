cmake_minimum_required(VERSION 3.24)

project(mc_internal LANGUAGES C CXX ASM)

# Export compile commands so clangd and other tools can find per-file flags
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(JNI REQUIRED COMPONENTS JVM)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

set(mc_internal_imgui_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/backends/imgui_impl_opengl3.cpp
)

set(mc_internal_jnif_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/external/jnif/src-libjnif/analysis.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/external/jnif/src-libjnif/base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/external/jnif/src-libjnif/model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/external/jnif/src-libjnif/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/external/jnif/src-libjnif/printer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/external/jnif/src-libjnif/stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/external/jnif/src-libjnif/writer.cpp
)

set(mc_internal_jnihook_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/src/classfile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/src/jnihook.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/src/jvm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/src/uuid.cpp
    ${mc_internal_jnif_sources}
)

# jnihook's upstream cmake assumes a configured java home, so we vendor the sources here.
add_library(mc_internal_jnihook STATIC ${mc_internal_jnihook_sources})
target_compile_features(mc_internal_jnihook PRIVATE cxx_std_23)
target_include_directories(mc_internal_jnihook
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/include
    PRIVATE
        ${JNI_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/src
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/jnihook/external/jnif/src-libjnif
)

# libmem is linked statically so the injected payload stays self-contained.
set(LIBMEM_BUILD_TESTS OFF CACHE BOOL "disable libmem tests" FORCE)
set(LIBMEM_DEEP_TESTS OFF CACHE BOOL "disable libmem deep tests" FORCE)
set(LIBMEM_BUILD_STATIC ON CACHE BOOL "build libmem as a static library" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/libmem)

# keystone lands in lib64 on some linux toolchains while libmem expects lib.
set(mc_internal_keystone_root
    ${CMAKE_BINARY_DIR}/deps/libmem/external/keystone-engine-prefix/src/keystone-engine-build/llvm
)
set(mc_internal_keystone_actual ${mc_internal_keystone_root}/lib64/libkeystone.a)
set(mc_internal_keystone_expected ${mc_internal_keystone_root}/lib/libkeystone.a)

add_custom_target(mc_internal_keystone_compat
    COMMAND ${CMAKE_COMMAND} -E make_directory ${mc_internal_keystone_root}/lib
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${mc_internal_keystone_actual} ${mc_internal_keystone_expected}
    DEPENDS keystone-engine
    BYPRODUCTS ${mc_internal_keystone_expected}
)

add_dependencies(libmem mc_internal_keystone_compat)

# imgui ships sources, not a cmake target.
add_library(mc_internal SHARED
    ${mc_internal_imgui_sources}
)

target_sources(mc_internal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/logging.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hook/glfw_bindings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hook/render_hook.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/jvm_attachment.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/jni_hook.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/bootstrap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sdk/jni_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sdk/math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sdk/minecraft.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/features/module_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/features/debug_hud_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/opengl_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/imgui_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/input_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/menu.cpp
)

target_compile_features(mc_internal PRIVATE cxx_std_23)
target_include_directories(mc_internal
    PRIVATE
        ${JNI_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/backends
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui/examples/libs/glfw/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/libmem/include
)

target_link_libraries(mc_internal
    PRIVATE
        mc_internal_jnihook
        libmem
        OpenGL::GL
        Threads::Threads
        dl
        pthread
)
