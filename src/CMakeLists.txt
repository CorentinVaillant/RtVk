
# Add source to this project's executable.
add_executable (RtVk 
  main.cpp
  ImageBuffer.cpp
  test.cpp
  graphics/vulkan_context.cpp
  graphics/Image.cpp
  graphics/utils.cpp
)
#remove -D NTEST to pass testing
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -D NDEBUG -D NVERB=5 -D NTEST -Wno-nullability-completeness")
set_property(TARGET RtVk PROPERTY CXX_STANDARD 20)
target_compile_features(RtVk PUBLIC cxx_std_23)
target_compile_definitions(RtVk PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(RtVk PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(RtVk PUBLIC glm Vulkan::Vulkan fmt::fmt stb_image stb_image_write SDL3::SDL3 vk-bootstrap::vk-bootstrap imgui fastgltf::fastgltf)
target_link_libraries(RtVk PRIVATE GPUOpen::VulkanMemoryAllocator)



target_precompile_headers(RtVk PUBLIC <optional> <vector> <memory> <string> <vector> <unordered_map> <glm/mat4x4.hpp>  <glm/vec4.hpp> <vulkan/vulkan.h>)

if(WIN32)
  add_custom_command(TARGET RtVk POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:RtVk> $<TARGET_FILE_DIR:RtVk>
    COMMAND_EXPAND_LISTS
    )
endif()