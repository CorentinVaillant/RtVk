
# Add source to this project's executable.
add_executable (RtVk 
  main.cpp
  ImageBuffer.cpp
  graphics/vulkan_context.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -D NDEBUG -D NVERB=5 -Wno-nullability-completeness")
set_property(TARGET RtVk PROPERTY CXX_STANDARD 20)
target_compile_features(RtVk PUBLIC cxx_std_23)
target_compile_definitions(RtVk PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(RtVk PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(RtVk PUBLIC vma glm Vulkan::Vulkan fmt::fmt stb_image stb_image_write SDL2::SDL2 vkbootstrap imgui fastgltf::fastgltf)

target_precompile_headers(RtVk PUBLIC <optional> <vector> <memory> <string> <vector> <unordered_map> <glm/mat4x4.hpp>  <glm/vec4.hpp> <vulkan/vulkan.h>)

if(WIN32)
  add_custom_command(TARGET RtVk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:RtVk> $<TARGET_FILE_DIR:RtVk>
    COMMAND_EXPAND_LISTS
    )
endif()