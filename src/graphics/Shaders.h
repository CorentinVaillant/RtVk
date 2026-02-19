#pragma once

#include "graphics/vulkan_context.h"
#include "types.h"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <span>
#include <vector>
#include <vulkan/vulkan_core.h>

class Shader {
public:
  Shader(VulkanContext &ctx, std::span<uint32_t> code_span)
      : _device(ctx._device) {

    uint32_t *code = code_span.data();
    size_t code_size = code_span.size_bytes();

    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .codeSize = code_size,
        .pCode = code,
    };

    VK_CHECK(
        vkCreateShaderModule(_device, &create_info, nullptr, &_shaderModule));
  }

  Shader(VulkanContext &ctx, std::vector<uint32_t> &&data)
      : Shader(ctx, std::span(data)) {}

  Shader(VulkanContext &ctx, const char *filepath)
      : Shader(ctx, load_shader_file(filepath)) {}

  NO_COPY(Shader);

  ~Shader() { vkDestroyShaderModule(_device, _shaderModule, nullptr); }

private:
  static std::vector<uint32_t> load_shader_file(const char *filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) {
      LOGERR("Could not open file : {}", filepath);
      return {};
    }

    size_t file_size = static_cast<size_t>(file.tellg());
    if (file_size == 0) {
      LOGERR("File ({}) is empty", filepath);
      return {};
    }

    std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data()), file_size);
    file.close();

    return buffer;
  }

public:
  VkShaderModule _shaderModule;

private:
  VkDevice _device;
};
