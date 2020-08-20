cmake_minimum_required(VERSION 3.10)
project(filament-samples C ASM)

if (FILAMENT_SKIP_SAMPLES)
    return()
endif()

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(GENERATION_ROOT ${CMAKE_CURRENT_BINARY_DIR})
set(RESOURCE_DIR  "${GENERATION_ROOT}/generated/resources")
set(MATERIAL_DIR  "${GENERATION_ROOT}/generated/material")
set(TEXTURE_DIR "${GENERATION_ROOT}/generated/texture")
set(RESOURCE_BINS)

# ==================================================================================================
# Build materials
# ==================================================================================================

set(MATERIAL_SRCS
        materials/aiDefaultMat.mat
        materials/bakedColor.mat
        materials/bakedTexture.mat
        materials/pointSprites.mat
        materials/aoPreview.mat
        materials/groundShadow.mat
        materials/heightfield.mat
        materials/sandboxCloth.mat
        materials/sandboxLit.mat
        materials/sandboxLitFade.mat
        materials/sandboxLitTransparent.mat
        materials/sandboxLitThinRefraction.mat
        materials/sandboxLitThinRefractionSsr.mat
        materials/sandboxLitSolidRefraction.mat
        materials/sandboxLitSolidRefractionSsr.mat
        materials/sandboxSpecGloss.mat
        materials/sandboxSubsurface.mat
        materials/sandboxUnlit.mat
        materials/texturedLit.mat)

if (CMAKE_CROSSCOMPILING)
    include(${IMPORT_EXECUTABLES})
endif()

file(MAKE_DIRECTORY ${MATERIAL_DIR})

foreach (mat_src ${MATERIAL_SRCS})
    get_filename_component(localname "${mat_src}" NAME_WE)
    get_filename_component(fullname "${mat_src}" ABSOLUTE)
    set(output_path "${MATERIAL_DIR}/${localname}.filamat")
    add_custom_command(
            OUTPUT ${output_path}
            COMMAND matc ${MATC_BASE_FLAGS} -o ${output_path} ${fullname}
            MAIN_DEPENDENCY ${mat_src}
            DEPENDS matc
            COMMENT "Compiling material ${mat_src} to ${output_path}"
    )
    list(APPEND RESOURCE_BINS ${output_path})
endforeach()

# ==================================================================================================
# Build sample resources
# ==================================================================================================

file(MAKE_DIRECTORY ${RESOURCE_DIR})

get_resgen_vars(${RESOURCE_DIR} resources)

add_custom_command(
        OUTPUT ${RESGEN_OUTPUTS}
        COMMAND resgen ${RESGEN_FLAGS} ${RESOURCE_BINS}
        DEPENDS resgen ${RESOURCE_BINS}
        COMMENT "Aggregating resources"
)

if (DEFINED RESGEN_SOURCE_FLAGS)
    set_source_files_properties(${RESGEN_SOURCE} PROPERTIES COMPILE_FLAGS ${RESGEN_SOURCE_FLAGS})
endif()

# CMake fails to invoke ar on Windows unless there is at least one C/C++ file in the library.
set(DUMMY_SRC "${RESOURCE_DIR}/dummy.c")
add_custom_command(OUTPUT ${DUMMY_SRC} COMMAND echo "//" > ${DUMMY_SRC})

add_library(sample-resources ${DUMMY_SRC} ${RESGEN_SOURCE})

# ==================================================================================================
# Invoke cmgen to build KTX files for the default IBL and skybox
# ==================================================================================================

set(CMGEN_ARGS --quiet --format=ktx --size=256 --extract-blur=0.1)

function(add_envmap SOURCE TARGET)
    set(source_envmap "${ROOT_DIR}/${SOURCE}")

    set(target_skybox   "${PROJECT_BINARY_DIR}/${TARGET}/${TARGET}_skybox.ktx")
    set(target_envmap   "${PROJECT_BINARY_DIR}/${TARGET}/${TARGET}_ibl.ktx")

    set(target_envmaps ${target_envmaps} ${target_skybox} PARENT_SCOPE)
    set(target_envmaps ${target_envmaps} ${target_envmap} PARENT_SCOPE)

    add_custom_command(OUTPUT ${target_skybox} ${target_envmap}
        COMMAND cmgen -x ${TARGET} ${CMGEN_ARGS} ${source_envmap}
        MAIN_DEPENDENCY ${source_envmap}
        DEPENDS cmgen
        COMMENT "Generating environment map ${target_envmap}")
endfunction()

add_envmap("third_party/environments/venetian_crossroads_2k.hdr" "venetian_crossroads_2k")

# ==================================================================================================
# Build suzanne resources
# ==================================================================================================

function(add_ktxfiles SOURCE TARGET EXTRA_ARGS)
    set(source_path "${ROOT_DIR}/${SOURCE}")
    set(target_path "${TEXTURE_DIR}/${TARGET}")
    set(MONKEY_FILES ${MONKEY_FILES} ${target_path} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND mipgen --quiet --strip-alpha ${EXTRA_ARGS} ${source_path} ${target_path}
        MAIN_DEPENDENCY ${source_path}
        DEPENDS mipgen
        COMMENT "Generating mip levels for ${source_path}")
endfunction()

function(add_pngfile SOURCE TARGET)
    set(source_path "${ROOT_DIR}/${SOURCE}")
    set(target_path "${TEXTURE_DIR}/${TARGET}")
    set(MONKEY_FILES ${MONKEY_FILES} ${target_path} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND ${CMAKE_COMMAND} -E copy ${source_path} ${target_path}
        MAIN_DEPENDENCY ${source_path})
endfunction()

function(add_mesh SOURCE TARGET)
    set(source_mesh "${ROOT_DIR}/${SOURCE}")
    set(target_mesh "${RESOURCE_DIR}/${TARGET}")
    set(MONKEY_FILES ${MESH_BINS} ${target_mesh} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_mesh}
        COMMAND filamesh --compress ${source_mesh} ${target_mesh}
        MAIN_DEPENDENCY ${source_mesh}
        DEPENDS filamesh)
endfunction()

#add_mesh("assets/models/monkey/monkey.obj" "suzanne.filamesh")

# Drill.
add_mesh("assets/models/drill/drillobj.obj" "drill.filamesh")
add_ktxfiles("assets/models/drill/drill_albedo.png" "drill_albedo_s3tc.ktx" "--compression=s3tc_rgb_dxt1")
add_ktxfiles("assets/models/drill/drill_roughness.png" "drill_roughness.ktx" "--grayscale")
add_ktxfiles("assets/models/drill/drill_metalness.png" "drill_metallic.ktx" "--grayscale")
add_pngfile("assets/models/drill/drill_normal.png" "drill_normal.png")

set(DRILL_RESOURCE_FILES
    ${TEXTURE_DIR}/drill_albedo_s3tc.ktx
    ${TEXTURE_DIR}/drill_roughness.ktx
    ${TEXTURE_DIR}/drill_metallic.ktx
    ${TEXTURE_DIR}/drill_normal.png
    ${RESOURCE_DIR}/drill.filamesh
)

get_resgen_vars(${RESOURCE_DIR} drill)

add_custom_command(
    OUTPUT ${RESGEN_OUTPUTS}
    COMMAND resgen ${RESGEN_FLAGS} ${DRILL_RESOURCE_FILES}
    DEPENDS resgen ${DRILL_RESOURCE_FILES}
)

if (DEFINED RESGEN_SOURCE_FLAGS)
    set_source_files_properties(${RESGEN_SOURCE} PROPERTIES COMPILE_FLAGS ${RESGEN_SOURCE_FLAGS})
endif()

add_library(suzanne-resources ${DUMMY_SRC} ${RESGEN_SOURCE})


# ==================================================================================================
# Build valve resources
# ==================================================================================================

function(add_valve_ktxfiles SOURCE TARGET EXTRA_ARGS)
    set(source_path "${ROOT_DIR}/${SOURCE}")
    set(target_path "${TEXTURE_DIR}/${TARGET}")
    set(VALVE_FILES ${VALVE_FILES} ${target_path} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND mipgen --quiet --strip-alpha ${EXTRA_ARGS} ${source_path} ${target_path}
        MAIN_DEPENDENCY ${source_path}
        DEPENDS mipgen
        COMMENT "Generating mip levels for ${source_path}")
endfunction()

function(add_valve_pngfile SOURCE TARGET)
    set(source_path "${ROOT_DIR}/${SOURCE}")
    set(target_path "${TEXTURE_DIR}/${TARGET}")
    set(VALVE_FILES ${VALVE_FILES} ${target_path} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND ${CMAKE_COMMAND} -E copy ${source_path} ${target_path}
        MAIN_DEPENDENCY ${source_path})
endfunction()

function(add_valve_mesh SOURCE TARGET)
    set(source_mesh "${ROOT_DIR}/${SOURCE}")
    set(target_mesh "${RESOURCE_DIR}/${TARGET}")
    set(VALVE_FILES ${MESH_BINS} ${target_mesh} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_mesh}
        COMMAND filamesh --compress ${source_mesh} ${target_mesh}
        MAIN_DEPENDENCY ${source_mesh}
        DEPENDS filamesh)
endfunction()

# Valve.
add_mesh("assets/models/valve/valve.obj" "valve.filamesh")
add_ktxfiles("assets/models/valve/valve_albedo.png" "valve_albedo_s3tc.ktx" "--compression=s3tc_rgb_dxt1")
add_ktxfiles("assets/models/valve/valve_roughness.png" "valve_roughness.ktx" "--grayscale")
add_ktxfiles("assets/models/valve/valve_metalness.png" "valve_metallic.ktx" "--grayscale")
add_pngfile("assets/models/valve/valve_normal.png" "valve_normal.png")

set(VALVE_RESOURCE_FILES
    ${TEXTURE_DIR}/valve_albedo_s3tc.ktx
    ${TEXTURE_DIR}/valve_roughness.ktx
    ${TEXTURE_DIR}/valve_metallic.ktx
    ${TEXTURE_DIR}/valve_normal.png
    ${RESOURCE_DIR}/valve.filamesh
)

get_resgen_vars(${RESOURCE_DIR} valve)

add_custom_command(
    OUTPUT ${RESGEN_OUTPUTS}
    COMMAND resgen ${RESGEN_FLAGS} ${VALVE_RESOURCE_FILES}
    DEPENDS resgen ${VALVE_RESOURCE_FILES}
)

if (DEFINED RESGEN_SOURCE_FLAGS)
    set_source_files_properties(${RESGEN_SOURCE} PROPERTIES COMPILE_FLAGS ${RESGEN_SOURCE_FLAGS})
endif()

add_library(valve-resources ${DUMMY_SRC} ${RESGEN_SOURCE})

# ==================================================================================================
# Build extinguisher resources
# ==================================================================================================

function(add_extinguisher_ktxfiles SOURCE TARGET EXTRA_ARGS)
    set(source_path "${ROOT_DIR}/${SOURCE}")
    set(target_path "${TEXTURE_DIR}/${TARGET}")
    set(EXTINGUISHER_FILES ${EXTINGUISHER_FILES} ${target_path} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND mipgen --quiet --strip-alpha ${EXTRA_ARGS} ${source_path} ${target_path}
        MAIN_DEPENDENCY ${source_path}
        DEPENDS mipgen
        COMMENT "Generating mip levels for ${source_path}")
endfunction()

function(add_extinguisher_pngfile SOURCE TARGET)
    set(source_path "${ROOT_DIR}/${SOURCE}")
    set(target_path "${TEXTURE_DIR}/${TARGET}")
    set(EXTINGUISHER_FILES ${EXTINGUISHER_FILES} ${target_path} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND ${CMAKE_COMMAND} -E copy ${source_path} ${target_path}
        MAIN_DEPENDENCY ${source_path})
endfunction()

function(add_extinguisher_mesh SOURCE TARGET)
    set(source_mesh "${ROOT_DIR}/${SOURCE}")
    set(target_mesh "${RESOURCE_DIR}/${TARGET}")
    set(EXTINGUISHER_FILES ${MESH_BINS} ${target_mesh} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_mesh}
        COMMAND filamesh --compress ${source_mesh} ${target_mesh}
        MAIN_DEPENDENCY ${source_mesh}
        DEPENDS filamesh)
endfunction()

# Extinguisher.
add_extinguisher_mesh("assets/models/extinguisher/extinguisher.obj" "extinguisher.filamesh")
add_extinguisher_ktxfiles("assets/models/extinguisher/extinguisher_albedo.png" "extinguisher_albedo_s3tc.ktx" "--compression=s3tc_rgb_dxt1")
add_extinguisher_ktxfiles("assets/models/extinguisher/extinguisher_roughness.png" "extinguisher_roughness.ktx" "--grayscale")
add_extinguisher_ktxfiles("assets/models/extinguisher/extinguisher_metalness.png" "extinguisher_metallic.ktx" "--grayscale")
add_extinguisher_pngfile("assets/models/extinguisher/extinguisher_normal.png" "extinguisher_normal.png")

set(EXTINGUISHER_RESOURCE_FILES
    ${TEXTURE_DIR}/extinguisher_albedo_s3tc.ktx
    ${TEXTURE_DIR}/extinguisher_roughness.ktx
    ${TEXTURE_DIR}/extinguisher_metallic.ktx
    ${TEXTURE_DIR}/extinguisher_normal.png
    ${RESOURCE_DIR}/extinguisher.filamesh
)

get_resgen_vars(${RESOURCE_DIR} extinguisher)

add_custom_command(
    OUTPUT ${RESGEN_OUTPUTS}
    COMMAND resgen ${RESGEN_FLAGS} ${EXTINGUISHER_RESOURCE_FILES}
    DEPENDS resgen ${EXTINGUISHER_RESOURCE_FILES}
)

if (DEFINED RESGEN_SOURCE_FLAGS)
    set_source_files_properties(${RESGEN_SOURCE} PROPERTIES COMPILE_FLAGS ${RESGEN_SOURCE_FLAGS})
endif()

add_library(extinguisher-resources ${DUMMY_SRC} ${RESGEN_SOURCE})

# ==================================================================================================
# Build rescue_randy resources
# ==================================================================================================

function(add_rescue_randy_ktxfiles SOURCE TARGET EXTRA_ARGS)
    set(source_path "${ROOT_DIR}/${SOURCE}")
    set(target_path "${TEXTURE_DIR}/${TARGET}")
    set(RESCUE_RANDY_FILES ${RESCUE_RANDY_FILES} ${target_path} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND mipgen --quiet --strip-alpha ${EXTRA_ARGS} ${source_path} ${target_path}
        MAIN_DEPENDENCY ${source_path}
        DEPENDS mipgen
        COMMENT "Generating mip levels for ${source_path}")
endfunction()

function(add_rescue_randy_pngfile SOURCE TARGET)
    set(source_path "${ROOT_DIR}/${SOURCE}")
    set(target_path "${TEXTURE_DIR}/${TARGET}")
    set(RESCUE_RANDY_FILES ${RESCUE_RANDY_FILES} ${target_path} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND ${CMAKE_COMMAND} -E copy ${source_path} ${target_path}
        MAIN_DEPENDENCY ${source_path})
endfunction()

function(add_rescue_randy_mesh SOURCE TARGET)
    set(source_mesh "${ROOT_DIR}/${SOURCE}")
    set(target_mesh "${RESOURCE_DIR}/${TARGET}")
    set(RESCUE_RANDY_FILES ${MESH_BINS} ${target_mesh} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_mesh}
        COMMAND filamesh --compress ${source_mesh} ${target_mesh}
        MAIN_DEPENDENCY ${source_mesh}
        DEPENDS filamesh)
endfunction()

# Rescue randy.
add_rescue_randy_mesh("assets/models/rescue_randy/rescue_randy.obj" "rescue_randy.filamesh")
add_rescue_randy_ktxfiles("assets/models/rescue_randy/rescue_randy_albedo.png" "rescue_randy_albedo_s3tc.ktx" "--compression=s3tc_rgb_dxt1")
add_rescue_randy_ktxfiles("assets/models/rescue_randy/rescue_randy_roughness.png" "rescue_randy_roughness.ktx" "--grayscale")
add_rescue_randy_ktxfiles("assets/models/rescue_randy/rescue_randy_metalness.png" "rescue_randy_metallic.ktx" "--grayscale")
add_rescue_randy_pngfile("assets/models/rescue_randy/rescue_randy_normal.png" "rescue_randy_normal.png")

set(RESCUE_RANDY_RESOURCE_FILES
    ${TEXTURE_DIR}/rescue_randy_albedo_s3tc.ktx
    ${TEXTURE_DIR}/rescue_randy_roughness.ktx
    ${TEXTURE_DIR}/rescue_randy_metallic.ktx
    ${TEXTURE_DIR}/rescue_randy_normal.png
    ${RESOURCE_DIR}/rescue_randy.filamesh
)

get_resgen_vars(${RESOURCE_DIR} rescue_randy)

add_custom_command(
    OUTPUT ${RESGEN_OUTPUTS}
    COMMAND resgen ${RESGEN_FLAGS} ${RESCUE_RANDY_RESOURCE_FILES}
    DEPENDS resgen ${RESCUE_RANDY_RESOURCE_FILES}
)

if (DEFINED RESGEN_SOURCE_FLAGS)
    set_source_files_properties(${RESGEN_SOURCE} PROPERTIES COMPILE_FLAGS ${RESGEN_SOURCE_FLAGS})
endif()

add_library(rescue_randy-resources ${DUMMY_SRC} ${RESGEN_SOURCE})

# ==================================================================================================
# Build pump resources
# ==================================================================================================

function(add_pump_ktxfiles SOURCE TARGET EXTRA_ARGS)
    set(source_path "${ROOT_DIR}/${SOURCE}")
    set(target_path "${TEXTURE_DIR}/${TARGET}")
    set(PUMP_FILES ${PUMP_FILES} ${target_path} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND mipgen --quiet --strip-alpha ${EXTRA_ARGS} ${source_path} ${target_path}
        MAIN_DEPENDENCY ${source_path}
        DEPENDS mipgen
        COMMENT "Generating mip levels for ${source_path}")
endfunction()

function(add_pump_pngfile SOURCE TARGET)
    set(source_path "${ROOT_DIR}/${SOURCE}")
    set(target_path "${TEXTURE_DIR}/${TARGET}")
    set(PUMP_FILES ${PUMP_FILES} ${target_path} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_path}
        COMMAND ${CMAKE_COMMAND} -E copy ${source_path} ${target_path}
        MAIN_DEPENDENCY ${source_path})
endfunction()

function(add_pump_mesh SOURCE TARGET)
    set(source_mesh "${ROOT_DIR}/${SOURCE}")
    set(target_mesh "${RESOURCE_DIR}/${TARGET}")
    set(PUMP_FILES ${MESH_BINS} ${target_mesh} PARENT_SCOPE)
    add_custom_command(
        OUTPUT ${target_mesh}
        COMMAND filamesh --compress ${source_mesh} ${target_mesh}
        MAIN_DEPENDENCY ${source_mesh}
        DEPENDS filamesh)
endfunction()

# Pump.
add_pump_mesh("assets/models/pump/pump.obj" "pump.filamesh")
add_pump_ktxfiles("assets/models/pump/pump_albedo.png" "pump_albedo_s3tc.ktx" "--compression=s3tc_rgb_dxt1")
add_pump_ktxfiles("assets/models/pump/pump_roughness.png" "pump_roughness.ktx" "--grayscale")
add_pump_ktxfiles("assets/models/pump/pump_metallic.png" "pump_metallic.ktx" "--grayscale")
add_pump_pngfile("assets/models/pump/pump_normal.png" "pump_normal.png")

set(PUMP_RESOURCE_FILES
    ${TEXTURE_DIR}/pump_albedo_s3tc.ktx
    ${TEXTURE_DIR}/pump_roughness.ktx
    ${TEXTURE_DIR}/pump_metallic.ktx
    ${TEXTURE_DIR}/pump_normal.png
    ${RESOURCE_DIR}/pump.filamesh
)

get_resgen_vars(${RESOURCE_DIR} pump)

add_custom_command(
    OUTPUT ${RESGEN_OUTPUTS}
    COMMAND resgen ${RESGEN_FLAGS} ${PUMP_RESOURCE_FILES}
    DEPENDS resgen ${PUMP_RESOURCE_FILES}
)

if (DEFINED RESGEN_SOURCE_FLAGS)
    set_source_files_properties(${RESGEN_SOURCE} PROPERTIES COMPILE_FLAGS ${RESGEN_SOURCE_FLAGS})
endif()

add_library(pump-resources ${DUMMY_SRC} ${RESGEN_SOURCE})


# ==================================================================================================
# Resources for gltf_viewer
# ==================================================================================================

add_library(gltf-resources ${DUMMY_SRC})

set(GLTF_VIEWER_RESOURCE_FILES
    ${ROOT_DIR}/third_party/models/DamagedHelmet/DamagedHelmet.glb
    ${MATERIAL_DIR}/groundShadow.filamat
)

get_resgen_vars(${RESOURCE_DIR} gltf_viewer)

add_custom_command(
        OUTPUT ${RESGEN_OUTPUTS}
        COMMAND resgen ${RESGEN_FLAGS} ${GLTF_VIEWER_RESOURCE_FILES}
        DEPENDS resgen ${GLTF_VIEWER_RESOURCE_FILES}
)

if (DEFINED RESGEN_SOURCE_FLAGS)
    set_source_files_properties(${RESGEN_SOURCE} PROPERTIES COMPILE_FLAGS ${RESGEN_SOURCE_FLAGS})
endif()

target_sources(gltf-resources PRIVATE ${RESGEN_SOURCE})

# ==================================================================================================
# Compiler flags
# ==================================================================================================

if (MSVC)
    set(COMPILER_FLAGS $<$<CONFIG:Release>:/fp:fast>)
else()
    set(COMPILER_FLAGS
            -Wno-extern-c-compat
            $<$<NOT:$<PLATFORM_ID:Linux>>:-Wno-address-of-packed-member>
            $<$<CONFIG:Release>:-ffast-math>)
endif()

# ==================================================================================================
# Test apps
# ==================================================================================================

function(add_demo NAME)
    include_directories(${GENERATION_ROOT})
    add_executable(${NAME} ${NAME}.cpp)
    target_link_libraries(${NAME} PRIVATE sample-resources filamentapp)
    target_compile_options(${NAME} PRIVATE ${COMPILER_FLAGS})
endfunction()

if (NOT ANDROID)
    add_demo(animation)
    add_demo(depthtesting)
    add_demo(frame_generator)
    add_demo(gltf_viewer)
    add_demo(gltf_instances)
    add_demo(heightfield)
    add_demo(hellopbr)
    add_demo(hellotriangle)
    add_demo(lightbulb)
    add_demo(material_sandbox)
    add_demo(point_sprites)
    add_demo(sample_cloth)
    add_demo(sample_full_pbr)
    add_demo(sample_normal_map)
    add_demo(shadowtest)
    add_demo(strobecolor)
    add_demo(suzanne)
    add_demo(texturedquad)
    add_demo(vbotest)
    add_demo(viewtest)

    # Sample app specific
    target_link_libraries(frame_generator PRIVATE imageio)
    target_link_libraries(gltf_viewer PRIVATE gltf-resources gltfio)
    target_link_libraries(gltf_instances PRIVATE gltf-resources gltfio)
    target_link_libraries(hellopbr PRIVATE filameshio suzanne-resources)
    target_link_libraries(sample_cloth PRIVATE filameshio)
    target_link_libraries(sample_normal_map PRIVATE filameshio)
    target_link_libraries(suzanne PRIVATE filameshio suzanne-resources valve-resources extinguisher-resources rescue_randy-resources pump-resources)

endif()

# ==================================================================================================
# Copy the MoltenVK dylibs and JSON on MacOS
# ==================================================================================================

if (APPLE AND NOT Vulkan_LIBRARY)
    message(STATUS "No Vulkan SDK was found, using prebuilt MoltenVK.")
    set(MOLTENVK_DIR "../third_party/moltenvk")
    configure_file(
            ${MOLTENVK_DIR}/libvulkan.1.dylib
            ${PROJECT_BINARY_DIR}/libvulkan.1.dylib COPYONLY)
    configure_file(
            ${MOLTENVK_DIR}/MoltenVK_icd.json
            ${PROJECT_BINARY_DIR}/MoltenVK_icd.json COPYONLY)
    configure_file(
            ${MOLTENVK_DIR}/libMoltenVK.dylib
            ${PROJECT_BINARY_DIR}/libMoltenVK.dylib COPYONLY)
    list(APPEND APP_SRCS
            ${PROJECT_BINARY_DIR}/libvulkan.1.dylib
            ${PROJECT_BINARY_DIR}/MoltenVK_icd.json
            ${PROJECT_BINARY_DIR}/libMoltenVK.dylib)
endif()

# ==================================================================================================
# Copy Assets
#
# This creates a structure like:
#
#    /lightbulb (etc)  sample app executable
#    /assets/fonts     copied from REPO/assets
#    /assets/models    copied from REPO/assets
#    /textures         copied from REPO/third_party
#
# ==================================================================================================

file(COPY ../third_party/textures DESTINATION ${PROJECT_BINARY_DIR})
add_custom_target(textures ALL DEPENDS textures)
add_dependencies(filament textures)

file(COPY ../assets DESTINATION ${PROJECT_BINARY_DIR}
        PATTERN "reference" EXCLUDE
        PATTERN "environments" EXCLUDE)
add_custom_target(assets ALL DEPENDS assets)
add_dependencies(filament assets)

add_custom_target(envs DEPENDS ${target_envmaps})
add_dependencies(filament envs)
