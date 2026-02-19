#pragma once

#include "graphics/Shaders.h"
#include "graphics/utils.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#include <vulkan/vulkan_core.h>

template <Descriptable PushCst> class ComputePipeline {
public:
  ComputePipeline(VulkanContext &ctx, DescriptorAllocator &allocator,
                  Shader &&compute_shader)
      : _descriptorSetLayout(
            PushCst::describe(ctx._device, VK_SHADER_STAGE_COMPUTE_BIT)),
        _ctxDevice(ctx._device) {

    _descriptorSet = allocator.allocate(_descriptorSetLayout);

    // init pipeline layout
    VkPushConstantRange push_cst_range = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PushCst),
    };

    VkPipelineLayoutCreateInfo layout_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = {},
        .setLayoutCount = 1,
        .pSetLayouts = &_descriptorSetLayout._descrSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_cst_range,
    };

    VK_CHECK(vkCreatePipelineLayout(_ctxDevice, &layout_create_info, nullptr,
                                    &_pipelineLayout));

    // creating the pipeline
    VkPipelineShaderStageCreateInfo stage_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = {}, // defaulted
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = compute_shader._shaderModule,
        .pName = stage_info.pName = "main",
        .pSpecializationInfo = {}, // defaulted
    };

    VkComputePipelineCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = {}, // defaulted
        .stage = stage_info,
        .layout = _pipelineLayout,
        .basePipelineHandle = {}, // no parent pipeline
        .basePipelineIndex = {},  // no parent pipeline
    };

    VK_CHECK(vkCreateComputePipelines(_ctxDevice, VK_NULL_HANDLE, 1,
                                      &create_info, nullptr, &_pipeline));
  }

  ~ComputePipeline() {
    vkDestroyPipelineLayout(_ctxDevice, _pipelineLayout, nullptr);
    vkDestroyPipeline(_ctxDevice, _pipeline, nullptr);
  }

private:
  DescriptorSetLayout _descriptorSetLayout;
  VkDescriptorSet _descriptorSet;
  VkPipelineLayout _pipelineLayout;
  VkPipeline _pipeline;

  VkDevice _ctxDevice;
};
