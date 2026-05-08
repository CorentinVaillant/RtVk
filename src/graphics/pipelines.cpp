#include "pipelines.h"
#include "glm/ext/vector_uint3.hpp"
#include "graphics/Buffer.h"
#include "graphics/raii_graphic.h"
#include "graphics/utils.h"
#include "graphics/vma_usage.h"
#include "types.h"
// -- ComputePipeline --

// -- Constructors
// public:
ComputePipeline::ComputePipeline(VulkanContext &ctx,
                                 PipelineDescriptor &descriptor,
                                 glm::uvec3 dispatch_groupe,
                                 VkPipelineCreateFlags flags /* = {} */)
    : _deviceCtx(ctx._device), _dispatchGroup(dispatch_groupe) {

  auto stages = descriptor.get_stages_infos();
  assert(stages.size() == 1);

  init_sampler(descriptor);
  init_descr_set_layout(ctx, descriptor);
  init_layout(descriptor);

  VkComputePipelineCreateInfo create_info = VkComputePipelineCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = {},
      .stage = stages[0],
      .layout = _layout,
      .basePipelineHandle = {}, // No inheritance
      .basePipelineIndex = {},
  };

  VK_CHECK(vkCreateComputePipelines(_deviceCtx, VK_NULL_HANDLE, 1, &create_info,
                                    nullptr, &_pipeline));
}

ComputePipeline::~ComputePipeline() {
  vkDestroyPipeline(_deviceCtx, _pipeline, nullptr);
  vkDestroyPipelineLayout(_deviceCtx, _layout, nullptr);
  vkDestroySampler(_deviceCtx, _sampler, nullptr);
  vkDestroyDescriptorSetLayout(_deviceCtx, _descrSetLayout, nullptr);
  ;
}

// -- Methods

void ComputePipeline::bind(VkCommandBuffer cmd, VkDescriptorSet descriptor) {
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, _layout, 0, 1,
                          &descriptor, 0, nullptr);
}

// private:
void ComputePipeline::init_sampler(PipelineDescriptor &descriptor) {
  VK_CHECK(vkCreateSampler(_deviceCtx, &descriptor._samplerInfo, nullptr,
                           &_sampler));
}

void ComputePipeline::init_descr_set_layout(VulkanContext &ctx,
                                            PipelineDescriptor &descriptor) {
  _descrSetLayout = descriptor.create_descriptor_binding(ctx, _sampler);
}

void ComputePipeline::init_layout(PipelineDescriptor &descriptor) {
  VkPipelineLayoutCreateInfo create_info = descriptor._layoutInfo;
  create_info.pSetLayouts = &_descrSetLayout;
  create_info.pPushConstantRanges = &descriptor._pushCstRange;

  VK_CHECK(vkCreatePipelineLayout(_deviceCtx, &create_info, nullptr, &_layout));
}

// -- RT pipeline --

RtPipeline::RtPipeline(VulkanContext &ctx, PipelineDescriptor &descriptor,
                       RtPipelineCreateInfos &pipeline_infos)
    : _ctxDevice(ctx._device) {

  // Pipeline creation

  auto stages = descriptor.get_stages_infos();
  assert(stages.size() >= 3);

  init_sampler(descriptor);
  init_descr_set_layout(ctx, descriptor);
  init_layout(descriptor);

  uint32_t group_count = pipeline_infos.groups.size();

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

  vkCreateRayTracingPipelinesKHR(_ctxDevice, VK_NULL_HANDLE, VK_NULL_HANDLE, 1,
                                 &create_info, nullptr, &_rtPipeline);

  // Shader binding table creation
  const VkPhysicalDeviceRayTracingPipelinePropertiesKHR &rt_props =
      ctx.get_physical_device_rt_pipeline_properties();

  uint32_t handle_size = rt_props.shaderGroupHandleSize;
  uint32_t handle_aligment = rt_props.shaderGroupHandleAlignment;
  uint32_t base_alignement = rt_props.shaderGroupBaseAlignment;

  auto align_up = [](uint32_t v, uint32_t a) { return (v + a - 1) & ~(a - 1); };

  uint32_t stride = align_up(handle_size, handle_aligment);

  uint32_t total_handle_size = handle_size * group_count;
  std::vector<uint8_t> handles(total_handle_size);
  vkGetRayTracingShaderGroupHandlesKHR(_ctxDevice, _rtPipeline, 0, group_count,
                                       total_handle_size, handles.data());

  constexpr VkBufferUsageFlags SBT_BUFFER_USAGE =
      VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VkDeviceSize raygen_entry = align_up(stride, base_alignement);
  VkDeviceSize miss_entry = align_up(stride, base_alignement);
  VkDeviceSize hit_entry =
      align_up(stride * pipeline_infos.hit_stride_count, base_alignement);

  VkDeviceSize raygen_size = raygen_entry * pipeline_infos.raygen_count;
  VkDeviceSize miss_size = miss_entry * pipeline_infos.miss_count;
  VkDeviceSize hit_size = hit_entry * (pipeline_infos.hit_count / pipeline_infos.hit_stride_count);

  VkDeviceSize sbt_size = raygen_size + miss_size + hit_size;

  _sbtBuffer = Buffer<uint8_t>(ctx, sbt_size, SBT_BUFFER_USAGE,
                               VMA_MEMORY_USAGE_CPU_TO_GPU);
  VkDeviceAddress sbt_address = _sbtBuffer->get_device_adresse(_ctxDevice);

  uint32_t non_hit = group_count - pipeline_infos.hit_count;

  // Raygen
  for (uint32_t i = 0; i < pipeline_infos.raygen_count; i++)
    _sbtBuffer->write(i * raygen_entry, handle_size,
                      handles.data() + i * handle_size);

  // Miss
  for (uint32_t i = pipeline_infos.raygen_count; i < non_hit; i++)
    _sbtBuffer->write(raygen_size +
                          (i - pipeline_infos.raygen_count) * miss_entry,
                      handle_size, handles.data() + i * handle_size);

  // Hit
  for (uint32_t i = non_hit; i < group_count; i++)
    _sbtBuffer->write(raygen_size + miss_size + (i - non_hit) * hit_entry,
                      handle_size, handles.data() + i * handle_size);

  _raygen_region = {sbt_address, raygen_entry, raygen_size};
  _miss_region = {sbt_address + raygen_size, miss_entry, miss_size};
  _hit_region = {sbt_address + raygen_size + miss_size, hit_entry, hit_size};
  _callable_region = {0, 0, 0}; // ~ No callable region for now, no need of it

  LOG(5, "raygen_region = {{ {}, {}, {}}}",sbt_address, raygen_entry, raygen_size);
  LOG(5, "_miss_region = {{ {}, {}, {}}}",sbt_address + raygen_size, miss_entry, miss_size);
  LOG(5, "_hit_region = {{ {}, {}, {}}}",sbt_address + raygen_size + miss_size, hit_entry, hit_size);
}

RtPipeline::~RtPipeline() {
  vkDestroyPipeline(_ctxDevice, _rtPipeline, nullptr);
  vkDestroyPipelineLayout(_ctxDevice, _layout, nullptr);
  vkDestroyDescriptorSetLayout(_ctxDevice, _desrSetLayout, nullptr);
  vkDestroySampler(_ctxDevice, _sampler, nullptr);
}

// -- Methods

void RtPipeline::bind(VkCommandBuffer cmd, VkDescriptorSet descriptor) {
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _rtPipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _layout,
                          0, 1, &descriptor, 0, nullptr);
}

// private:
void RtPipeline::init_sampler(PipelineDescriptor &descriptor) {
  VK_CHECK(vkCreateSampler(_ctxDevice, &descriptor._samplerInfo, nullptr,
                           &_sampler));
}

void RtPipeline::init_descr_set_layout(VulkanContext &ctx,
                                       PipelineDescriptor &descriptor) {
  _desrSetLayout = descriptor.create_descriptor_binding(ctx, _sampler);
}

void RtPipeline::init_layout(PipelineDescriptor &descriptor) {
  VkPipelineLayoutCreateInfo create_info = descriptor._layoutInfo;
  create_info.pSetLayouts = &_desrSetLayout;
  create_info.pPushConstantRanges = &descriptor._pushCstRange;

  VK_CHECK(vkCreatePipelineLayout(_ctxDevice, &create_info, nullptr, &_layout));
}