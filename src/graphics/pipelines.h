#pragma once
#include "glm/ext/vector_uint3.hpp"
#include "graphics/PipelineDescriptor.h"
#include "graphics/Shaders.h"
#include "graphics/vulkan_context.h"
#include "types.h"
#include <cassert>
#include <concepts>
#include <cstdint>
#include <volk.h>

// -- ComputePipeline --

class ComputePipeline {
  // -- Constructors --
public:
  ComputePipeline(VulkanContext &ctx, PipelineDescriptor &descriptor,
                  glm::uvec3 dispatch_groupe, VkPipelineCreateFlags flags = {});

  NO_COPY(ComputePipeline);

  ~ComputePipeline();

  // -- Getters --
  VkSampler get_sampler() { return _sampler; }

  // -- Methods --

  template <typename PushCst>
    requires std::copy_constructible<PushCst>
  void dispatch(VkCommandBuffer cmd, VkDescriptorSet descriptor,
                PushCst push_cst, glm::uvec3 groups) {
    bind(cmd, descriptor);

    vkCmdPushConstants(cmd, _layout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
                       sizeof(PushCst), &push_cst);

    vkCmdDispatch(cmd, groups.x / _dispatchGroup.x, groups.y / _dispatchGroup.y,
                  groups.z / _dispatchGroup.z);
  }

private:
  void init_sampler(PipelineDescriptor &descriptor);
  void init_descr_set_layout(VulkanContext &ctx,
                             PipelineDescriptor &descriptor);
  void init_layout(PipelineDescriptor &descriptor);

  void bind(VkCommandBuffer cmd, VkDescriptorSet descriptor);
  // -- Attributs --
private:
  VkDevice _deviceCtx;
  VkDescriptorSetLayout _descrSetLayout;
  VkSampler _sampler;
  VkPipelineLayout _layout;
  glm::uvec3 _dispatchGroup;

  VkPipeline _pipeline;
};

// -- RT pipeline --

struct RtPipelineCreateInfos {
  uint32_t max_rt_depth = 10;
  std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
};

class RtPipeline {
public:
  NO_COPY(RtPipeline);

  RtPipeline(VulkanContext &ctx, PipelineDescriptor &descriptor,
             RtPipelineCreateInfos &pipeline_infos)
      : _ctxDevice(ctx._device) {

    auto stages = descriptor.get_stages_infos();
    assert(stages.size() >= 3);

    init_sampler(descriptor);
    init_descr_set_layout(ctx, descriptor);
    init_layout(descriptor);

    uint32_t group_count = static_cast<uint32_t>(pipeline_infos.groups.size());

    VkRayTracingPipelineCreateInfoKHR create_info{
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = {}, // ?
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .groupCount = group_count,
        .pGroups = pipeline_infos.groups.data(),
        .maxPipelineRayRecursionDepth = pipeline_infos.max_rt_depth,
        .pLibraryInfo = {},      //| no lib
        .pLibraryInterface = {}, //|
        .pDynamicState = {},
        .layout = _layout,
        .basePipelineHandle = {}, //| No inheritance
        .basePipelineIndex = {},  //|
    };

    vkCreateRayTracingPipelinesKHR(_ctxDevice, VK_NULL_HANDLE, VK_NULL_HANDLE,
                                   1, &create_info, nullptr, &_rtPipeline);
  }

  ~RtPipeline() {
    vkDestroyPipeline(_ctxDevice, _rtPipeline, nullptr);
    vkDestroyPipelineLayout(_ctxDevice, _layout, nullptr);
    vkDestroyDescriptorSetLayout(_ctxDevice, _desrSetLayout, nullptr);
    vkDestroySampler(_ctxDevice, _sampler, nullptr);
  }

  // -- Methods --

private:
  void init_sampler(PipelineDescriptor &descriptor) {
    VK_CHECK(vkCreateSampler(_ctxDevice, &descriptor._samplerInfo, nullptr,
                             &_sampler));
  }

  void init_descr_set_layout(VulkanContext &ctx,
                             PipelineDescriptor &descriptor) {
    _desrSetLayout = descriptor.create_descriptor_binding(ctx, _sampler);
  }

  void init_layout(PipelineDescriptor &descriptor) {
    VkPipelineLayoutCreateInfo create_info = descriptor._layoutInfo;
    create_info.pSetLayouts = &_desrSetLayout;
    create_info.pPushConstantRanges = &descriptor._pushCstRange;

    VK_CHECK(
        vkCreatePipelineLayout(_ctxDevice, &create_info, nullptr, &_layout));
  }

private:
  VkDevice _ctxDevice;

  VkSampler _sampler;
  VkDescriptorSetLayout _desrSetLayout;
  VkPipelineLayout _layout;

  VkPipeline _rtPipeline;
};
