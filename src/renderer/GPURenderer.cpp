#include "GPURenderer.h"

#include <volk.h>
#include <vulkan/vulkan_core.h>

#include "graphics/Image.h"
#include "graphics/pipelines.h"
#include "graphics/raii_graphic.h"
#include "graphics/utils.h"
#include "graphics/vulkan_context.h"

#include "hittables/Sphere.h"
#include "shaders/simple_rt.slang.h"

GPURenderer::GPURenderer(VulkanContext &ctx, ImageBuffer &&img_buffer)
    : Renderer(std::move(img_buffer)), _ctx(ctx),
      _pipeline(create_pipeline(ctx)), _descr_aloc(create_descr_aloc(ctx)) {}

// -- Methods --

RendererPushCst GPURenderer::get_push_cst(const Scene &scene) const {
  return RendererPushCst{
      ._cam = scene.camera,
      ._lightDir = glm::vec4(1, 0, 0, 0),
      ._renderWidth = static_cast<float>(_imgBuffer.get_width()),
      ._renderHeight = static_cast<float>(_imgBuffer.get_height()),
  };
}

RendererUniforms GPURenderer::get_uniform(const Scene &scene) const {
  return RendererUniforms{._camRenderInfo = scene.camera.get_render_info(
                              static_cast<float>(_imgBuffer.get_width()),
                              static_cast<float>(_imgBuffer.get_height()))};
}

// -- Statics

RtPipeline GPURenderer::create_pipeline(VulkanContext &ctx) {
  PipelineDescriptor pipeline_descr;

  pipeline_descr.add_binding(0, StorageImage, Raygen)
      .add_binding(1, AccelerationStruct, Raygen)
      .add_binding(2, UniformBuffer, Raygen)
      .add_binding(3, StorageBuffer, {Raygen,Intersection, ClosestHit}); //! remove Raygen

  pipeline_descr.set_push_cst(ALL_RT_STAGE, 0, sizeof(RendererPushCst));

  // Shaders
  Shader rendering_shader = Shader(ctx, SIMPLE_RT_SPIRV);
  pipeline_descr.add_shader_stage(Raygen, rendering_shader, "rayGen")
      .add_shader_stage(Miss, rendering_shader, "miss")
      .add_shader_stage(ClosestHit, rendering_shader, "closestHit")
      .add_shader_stage(Intersection, rendering_shader, "sphereIntersection");

  // Shader groups
  RtPipelineCreateInfos create_info;
  create_info.max_rt_depth = 10;
  //! hardcoded for now...
  create_info.groups.resize(3);
  create_info.groups[0] = VkRayTracingShaderGroupCreateInfoKHR{
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
      .pNext = nullptr,
      .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
      .generalShader = 0, // raygen stage
      .closestHitShader = VK_SHADER_UNUSED_KHR,
      .anyHitShader = VK_SHADER_UNUSED_KHR,
      .intersectionShader = VK_SHADER_UNUSED_KHR,
  };

  create_info.groups[1] = VkRayTracingShaderGroupCreateInfoKHR{
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
      .pNext = nullptr,
      .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
      .generalShader = 1,                       // miss
      .closestHitShader = VK_SHADER_UNUSED_KHR, // miss
      .anyHitShader = VK_SHADER_UNUSED_KHR,
      .intersectionShader = VK_SHADER_UNUSED_KHR,
  };

  create_info.groups[2] = VkRayTracingShaderGroupCreateInfoKHR{
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
      .pNext = nullptr,
      .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
      .generalShader = VK_SHADER_UNUSED_KHR,
      .closestHitShader = 2, // closestHit
      .anyHitShader = VK_SHADER_UNUSED_KHR,
      .intersectionShader = 3, // sphereIntersection
  };

  return RtPipeline(ctx, pipeline_descr, create_info);
}

DescriptorAllocator GPURenderer::create_descr_aloc(VulkanContext &ctx) {

  DescriptorAllocator::PoolSizeRatio sizes_ratio[] = {
      {.type = StorageImage, .ratio = 3},
      {.type = AccelerationStruct, .ratio = 5},
      {.type = UniformBuffer, .ratio = 1},
      {.type = StorageBuffer, .ratio = 4},
  };

  return DescriptorAllocator(ctx, 10, sizes_ratio);
}

// -- Renderer impl
void GPURenderer::render(const Scene &scene) {

  Tlas tlas = scene._accStruct->get_gpu_struct(_ctx);

  VkDescriptorSetLayout descr_set_layout = _pipeline.get_descr_set_layout();
  Raii_VkDescriptorSet descr_set = _descr_aloc.allocate(descr_set_layout);

  auto push_cst = get_push_cst(scene);

  glm::uvec3 draw_region = {_imgBuffer.get_width(), _imgBuffer.get_height(), 1};

  Image result_img(_ctx, {draw_region.x, draw_region.y, draw_region.z}, RGBA,
                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                   General);

  Buffer<RendererUniforms> uniform_buffer = Buffer<RendererUniforms>(
      _ctx, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  auto uniform = get_uniform(scene);
  uniform_buffer.write(1, &uniform);

  std::vector<Buffer<>> primitive_buffers = scene._accStruct->upload_to_buffers(_ctx); // ! Probably probleme here > buffer in shader is size 0

  DescriptorWriter writter;
  result_img.write(writter, 0, _pipeline.get_sampler(), StorageImage);

  auto acc_write_descr_alloc = tlas.get_write_descr_alloc();
  tlas.get_buffer().write_into_descriptor(
      writter, 1, 1, 0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
      &acc_write_descr_alloc);

  uniform_buffer.write_into_descriptor(writter, 2, 1, 0,
                                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

  for (auto &buffer : primitive_buffers) { // ! Hardcoded for now

    buffer.write_into_descriptor(writter, 3, 2 * sizeof(Sphere) , 0,
                                 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
  }

  writter.update_set(_ctx._device, *descr_set);

  _ctx.immediate_submit([draw_region, &push_cst, &descr_set, this](auto cmd) {
    _pipeline.dispatch(cmd, *descr_set, draw_region, push_cst);
  });

  _imgBuffer.read_from_gpu(_ctx, result_img);
}
