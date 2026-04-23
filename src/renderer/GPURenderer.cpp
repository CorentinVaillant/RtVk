#include "GPURenderer.h"

#include <volk.h>
#include <vulkan/vulkan_core.h>

#include "graphics/Image.h"
#include "graphics/Shaders.h"
#include "graphics/pipelines.h"
#include "graphics/raii_graphic.h"
#include "graphics/utils.h"
#include "graphics/vma_usage.h"
#include "graphics/vulkan_context.h"

#include "renderer/renderer_utils.h"

#include "shaders/closest_hit.slang.h"
#include "shaders/intersections.slang.h"
#include "shaders/simple_rt.slang.h"

GPURenderer::GPURenderer(VulkanContext &ctx, ImageBuffer &&img_buffer)
    : Renderer(std::move(img_buffer)), _ctx(ctx),
      _pipeline(create_pipeline(ctx)), _descr_aloc(create_descr_aloc(ctx)) {}

// -- Methods --

RendererPushCst GPURenderer::get_push_cst(const Scene &scene) const {
  return RendererPushCst{
      ._cam = scene._camera[scene._active_camera],
      ._renderWidth = static_cast<float>(_imgBuffer.get_width()),
      ._renderHeight = static_cast<float>(_imgBuffer.get_height()),
      ._time = scene._time,
      .max_depth = 5,
      .spp = 250,
  };
}

RendererUniforms GPURenderer::get_uniform(const Scene &scene) const {
  return RendererUniforms{
      ._camRenderInfo = scene._camera[scene._active_camera].get_render_info(
          static_cast<float>(_imgBuffer.get_width()),
          static_cast<float>(_imgBuffer.get_height()))};
}

GPURenderer::RenderBuffers
GPURenderer::make_render_buffer(const Scene &scene) const {

  RenderBuffers result{
      .uniforms =
          Buffer<RendererUniforms>(_ctx, 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VMA_MEMORY_USAGE_CPU_TO_GPU),
      .materials = Buffer<Material>(_ctx, scene._materials.size(),
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                    VMA_MEMORY_USAGE_CPU_TO_GPU),
      .material_size = scene._materials.size(),
  };

  RendererUniforms uniform = get_uniform(scene);
  result.uniforms.write(1, &uniform);
  result.materials.write(scene._materials);

  return result;
}

// -- Statics

RtPipeline GPURenderer::create_pipeline(VulkanContext &ctx) {
  PipelineDescriptor pipeline_descr;

  pipeline_descr.add_binding(RESULT_IMAGE_BINDING, StorageImage, Raygen)
      .add_binding(SCENE_TLAS_BINDING, AccelerationStruct, {Raygen, ClosestHit})
      .add_binding(LIGHT_TLAS_BINDING, AccelerationStruct, {Raygen, ClosestHit})
      .add_binding(UNIFORMS_BINDING, UniformBuffer, Raygen)
      .add_binding(MATERIAL_BINDING, StorageBuffer, ClosestHit)
      .add_binding(INSTANCES_BINDING, StorageBuffer, ClosestHit)
      .add_binding(VERTEX_BINDING, StorageBuffer, ClosestHit)
      .add_binding(INDEX_BINDING, StorageBuffer, ClosestHit)
      .add_binding(LIGHT_BUFFER_BINDING, StorageBuffer, ALL_RT_STAGE);

  pipeline_descr.set_push_cst(ALL_RT_STAGE, 0, sizeof(RendererPushCst));

  // Shaders
  Shader main_shader = Shader(ctx, SIMPLE_RT_SPIRV);
  Shader closest_hit = Shader(ctx, CLOSEST_HIT_SPIRV);
  Shader intersection = Shader(ctx, INTERSECTIONS_SPIRV);
  pipeline_descr
      .add_shader_stage(Raygen, main_shader, "rayGen")                   /*0*/
      .add_shader_stage(Miss, main_shader, "miss")                       /*1*/
      .add_shader_stage(ClosestHit, closest_hit, "meshHit")              /*2*/
      .add_shader_stage(Intersection, intersection, "lightIntersection") /*3*/
      .add_shader_stage(ClosestHit, closest_hit, "lightHit")             /*4*/
      ;

  // Shader groups
  RtPipelineCreateInfos create_info;
  create_info.max_rt_depth = 10;
  create_info.groups.resize(4);
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
      .generalShader = 1, // miss
      .closestHitShader = VK_SHADER_UNUSED_KHR,
      .anyHitShader = VK_SHADER_UNUSED_KHR,
      .intersectionShader = VK_SHADER_UNUSED_KHR,
  };

  create_info.groups[2] = VkRayTracingShaderGroupCreateInfoKHR{
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
      .pNext = nullptr,
      .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
      .generalShader = VK_SHADER_UNUSED_KHR,
      .closestHitShader = 2, // closestHit for Triangle
      .anyHitShader = VK_SHADER_UNUSED_KHR,
      .intersectionShader = VK_SHADER_UNUSED_KHR,
  };

  create_info.groups[3] = VkRayTracingShaderGroupCreateInfoKHR{
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
      .pNext = nullptr,
      .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
      .generalShader = VK_SHADER_UNUSED_KHR,
      .closestHitShader = 4,
      .anyHitShader = VK_SHADER_UNUSED_KHR,
      .intersectionShader = 3,
  };

  return RtPipeline(ctx, pipeline_descr, create_info);
}

DescriptorAllocator GPURenderer::create_descr_aloc(VulkanContext &ctx) {

  DescriptorAllocator::PoolSizeRatio sizes_ratio[] = {
      {.type = StorageImage, .ratio = 3},
      {.type = AccelerationStruct, .ratio = 5},
      {.type = UniformBuffer, .ratio = 2},
      {.type = StorageBuffer, .ratio = 6},
  };

  return DescriptorAllocator(ctx, 1, sizes_ratio);
}

// -- Renderer impl
void GPURenderer::render(const Scene &scene) {
  VkDescriptorSetLayout descr_set_layout = _pipeline.get_descr_set_layout();
  Raii_VkDescriptorSet descr_set = _descr_aloc.allocate(descr_set_layout);

  RendererPushCst push_cst = get_push_cst(scene);

  glm::uvec3 draw_region = {_imgBuffer.get_width(), _imgBuffer.get_height(), 1};

  Image result_img(_ctx, {draw_region.x, draw_region.y, draw_region.z}, RGBA,
                   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                   General);

  RenderBuffers buffers = make_render_buffer(scene);
  DescriptorWriter writter;
  result_img.write(writter, 0, _pipeline.get_sampler(), StorageImage);

  UploadedAccStruct gpu_acc_struct =
      scene._collection.upload_to_gpu(_ctx, writter, scene._materials);

  auto acc_write_descr_alloc_scene = gpu_acc_struct.scene.get_write_descr_alloc();
  auto acc_write_descr_alloc_lights = gpu_acc_struct.lights_scene.get_write_descr_alloc();

  gpu_acc_struct.scene.get_buffer().write_into_descriptor(
      writter, SCENE_TLAS_BINDING, 1, 0,
      VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &acc_write_descr_alloc_scene);

  gpu_acc_struct.lights_scene.get_buffer().write_into_descriptor(
      writter, LIGHT_TLAS_BINDING, 1, 0,
      VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, &acc_write_descr_alloc_lights);

  buffers.uniforms.write_into_descriptor(writter, UNIFORMS_BINDING, 1, 0,
                                         VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

  buffers.materials.write_into_descriptor(writter, MATERIAL_BINDING,
                                          buffers.material_size, 0,
                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

  writter.update_set(_ctx._device, *descr_set);

  _ctx.immediate_submit([draw_region, &push_cst, &descr_set, this](auto cmd) {
    _pipeline.dispatch(cmd, *descr_set, draw_region, push_cst);
  });

  _imgBuffer.read_from_gpu(_ctx, result_img);
}
