cmake_minimum_required(VERSION 3.8)

add_library(
    ${PROJECT_NAME} 
    STATIC 
    src/gpu_bitset.cpp
    src/programs.cpp
    src/programs/erase_from_pool_program.cpp
    src/programs/insert_into_pool_program.cpp
    src/programs/remove_erased_ids_program.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC gl_classes)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_options(
    ${PROJECT_NAME}
    PUBLIC 
    # glm::vec4().xyz , etc..
    -DGLM_FORCE_SWIZZLE 
    -DGLM_FORCE_INLINE 
)
