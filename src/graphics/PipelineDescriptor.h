#pragma once

#include "graphics/Shaders.h"
#include "graphics/utils.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#include <vector>
#include <vulkan/vulkan_core.h>


class PipelineDescriptor {
public:
  PipelineDescriptor() {
    set_no_push_constant();
    set_default_layout();
    set_default_sampler();
  }

  // -- Setters --

  void set_push_cst(ShaderStages stages, uint32_t offset, uint32_t size) {
    _pushCstRange = VkPushConstantRange{
        .stageFlags = stages._vkShaderStageFlags,
        .offset = offset,
        .size = size,
    };
  }

  PipelineDescriptor& add_shader_stage(SingleShaderStage stage, Shader &shader,
                        const char *main_name = "main") {

    _shaderStages.emplace_back(VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .stage = static_cast<VkShaderStageFlagBits>(stage),
        .module = shader._shaderModule,
        .pName = main_name,
        .pSpecializationInfo = nullptr,
    });
    return *this;
  }

  PipelineDescriptor& add_binding(uint32_t binding, DescriptorType descr_type,
                   ShaderStages stages, uint32_t descr_count = 1) {
    _bindings.emplace_back(VkDescriptorSetLayoutBinding{
        .binding = binding,
        .descriptorType = static_cast<VkDescriptorType>(descr_type),
        .descriptorCount = descr_count,
        .stageFlags = stages._vkShaderStageFlags,
        .pImmutableSamplers = {}, // init during creation
    });

    return *this;
  }

  // -- Defaulters --

  PipelineDescriptor& set_default_sampler() {
    _samplerInfo = VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_NEAREST,
        // ...
    };

    return *this;
  }

  PipelineDescriptor& set_default_layout() {
    _layoutInfo = VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .setLayoutCount = 1,
        .pSetLayouts = {}, // init during creation
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &_pushCstRange,
    };

    return *this;
  }

  PipelineDescriptor& set_no_push_constant() {
    _pushCstRange = VkPushConstantRange{
        .stageFlags = 0,
        .offset = 0,
        .size = 0,
    };

    return *this;
  }

  PipelineDescriptor& clear_bindings() { _bindings.clear(); return *this;}
  PipelineDescriptor& clear_shader_stage() { _shaderStages.clear(); return *this;}

  // -- Getters --
  std::span<VkPipelineShaderStageCreateInfo> get_stages_infos() {
    return _shaderStages;
  }

  // -- Methods --

  VkDescriptorSetLayout
  create_descriptor_binding(VulkanContext &ctx, const VkSampler& sampler,
                            VkDescriptorSetLayoutCreateFlags flags = {}) {

    for(auto& bind : _bindings)
      bind.pImmutableSamplers  = &sampler;

    VkDescriptorSetLayoutCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = flags,
      .bindingCount = static_cast<uint32_t>(_bindings.size()),
      .pBindings = _bindings.data(),
    };
    VkDescriptorSetLayout result;
    VK_CHECK(vkCreateDescriptorSetLayout(ctx._device, &create_info, nullptr, &result));
    return result;
  }

  // -- Attributs --
public:
  VkSamplerCreateInfo _samplerInfo;
  VkPipelineLayoutCreateInfo _layoutInfo;
  VkPushConstantRange _pushCstRange;

private:
  std::vector<VkDescriptorSetLayoutBinding> _bindings;
  std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
};