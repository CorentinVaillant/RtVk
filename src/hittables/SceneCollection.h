#pragma once

#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "graphics/Tlas.h"
#include "graphics/vma_usage.h"
#include "hittables/Light.h"
#include "hittables/Model.h"
#include "hittables/TriangleRef.h"
#include "renderer/renderer_utils.h"
#include "types.h"
#include <vector>

struct UploadedAccStruct {
  std::optional<Buffer<>> instance_buffer;
  std::vector<Buffer<>> primitive_buffers;
  std::optional<Buffer<>> vertex_buffer;
  std::optional<Buffer<>> index_buffer;
  std::vector<Blas> blas;
  Tlas scene;
  Tlas lights_scene;
};

class SceneCollection {
  // -- Attributs --
private:
  std::vector<Vertex> _vertices;
  std::vector<Model> _models;
  std::vector<Light> _punctualLights;
  std::vector<size_t> _mesh_vertex_offsets = {};

  // -- Constructors --
public:
  SceneCollection(fastgltf::Asset &assets, fastgltf::Scene &scene) {
    LOG(5, "Loading Scene {}", scene.name);
    load_scene_lights(_punctualLights, assets, scene);
    load_vertices(assets);
    load_models(assets, scene);
  }

  SceneCollection(SceneCollection &&rval)
      : _vertices(std::move(rval._vertices)), _models(std::move(rval._models)),
        _punctualLights(std::move(rval._punctualLights)),
        _mesh_vertex_offsets(std::move(rval._mesh_vertex_offsets)) {}

  SceneCollection &operator=(SceneCollection &&rval) {
    if (this != &rval) {
      this->_vertices = std::move(rval._vertices);
      this->_punctualLights = std::move(rval._punctualLights);
      this->_models = std::move(rval._models);
      this->_mesh_vertex_offsets = std::move(rval._mesh_vertex_offsets);
    }
    return *this;
  }

  // -- Methods --
private:
  void load_vertices(fastgltf::Asset &assets) {
    _mesh_vertex_offsets.reserve(assets.meshes.size());

    for (auto &mesh : assets.meshes) {
      _mesh_vertex_offsets.push_back(_vertices.size());

      for (auto &primitive : mesh.primitives) {
        auto pos_it = primitive.findAttribute("POSITION");
        if (pos_it == primitive.attributes.end())
          continue;

        auto nor_it = primitive.findAttribute("NORMAL");
        auto uv_it = primitive.findAttribute("TEXCOORD_0");

        auto &pos_acc = assets.accessors[pos_it->accessorIndex];
        size_t base = _vertices.size();
        _vertices.resize(base + pos_acc.count);

        // Load positions
        fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
            assets, pos_acc, [&](fastgltf::math::fvec3 v, size_t i) {
              _vertices[base + i].position = {v[0], v[1], v[2]};
            });

        // Load normals
        if (nor_it != primitive.attributes.end()) {
          auto &nor_acc = assets.accessors[nor_it->accessorIndex];
          fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
              assets, nor_acc, [&](fastgltf::math::fvec3 n, size_t i) {
                _vertices[base + i].normal = {n[0], n[1], n[2]};
              });
        }

        // Load UVs
        if (uv_it != primitive.attributes.end()) {
          auto &uv_acc = assets.accessors[uv_it->accessorIndex];
          fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
              assets, uv_acc, [&](fastgltf::math::fvec2 uv, size_t i) {
                _vertices[base + i].u = uv[0];
                _vertices[base + i].v = uv[1];
              });
        }
      }
    }
  }

  void load_models(fastgltf::Asset &assets, fastgltf::Scene &scene) {
    std::span<Vertex> vertex_span(_vertices);
    std::vector<Model> models_vec;
    models_vec.reserve(scene.nodeIndices.size());
    for (size_t node_idx : scene.nodeIndices)
      models_vec.emplace_back(assets, assets.nodes[node_idx], vertex_span,
                              _mesh_vertex_offsets);
    _models = std::move(models_vec);
  }

  static void load_scene_lights(std::vector<Light> &out, fastgltf::Asset &asset,
                                fastgltf::Scene &scene) {
    for (size_t node_idx : scene.nodeIndices)
      load_scene_lights(out, asset, asset.nodes[node_idx]);

    out.push_back(LightPoint{Color{0}, glm::vec3{-9999}, 0.1});
  }

  static void load_scene_lights(std::vector<Light> &out, fastgltf::Asset &asset,
                                fastgltf::Node &node,
                                glm::mat4 parent_trans = glm::mat4(1)) {

    glm::mat4 local_trans = gltf_transform_to_glm(node.transform);
    glm::mat4 world_trans = parent_trans * local_trans;
    if (node.lightIndex.has_value()) {
      auto &gltf_light = asset.lights[node.lightIndex.value()];

      if (gltf_light.type == fastgltf::LightType::Point) {

        auto e = gltf_light.color * gltf_light.intensity;
        glm::vec4 a = world_trans * glm::vec4(0, 0, 0, 1);
        glm::vec3 pos = xyz(a);
        float radius = std::max(gltf_light.range.value_or(1e-1), 0.f);

        out.emplace_back(LightPoint{
            .emission = glm::vec4{e[0], e[1], e[2], 0.f},
            .position = pos,
            .radius = radius,
        });
        LOG(5, "Loaded light : {}", node.name);
      } else if (gltf_light.type == fastgltf::LightType::Directional) {
        glm::vec4 local_dir{0, 0, -1, 0};
        glm::vec4 world_dir = world_trans * local_dir;
        glm::vec3 direction = glm::normalize(glm::vec3(world_dir));

        auto e = gltf_light.color * gltf_light.intensity;

        out.emplace_back(LightSun{
            .emission = glm::vec4{e[0], e[1], e[2], 0.f},
            .forward_vec = direction,
            .angle = 0.5f, // hardcoded, gltf does not provide smt like that

        });
      }
    }
    for (size_t child_idx : node.children) {
      load_scene_lights(out, asset, asset.nodes[child_idx], world_trans);
    }
  }

public:
  // vulkan
  UploadedAccStruct upload_to_gpu(VulkanContext &ctx,
                                  DescriptorWriter &writter) const {

    // Upload vertices into GPU
    Buffer<> vbuff(
        ctx, _vertices.size() * sizeof(Vertex),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VMA_MEMORY_USAGE_CPU_TO_GPU);

    vbuff.write(0, _vertices.size() * sizeof(Vertex),
                reinterpret_cast<const uint8_t *>(_vertices.data()));

    // Upload indices into GPU
    size_t nb_triangle = 0;
    for (const auto &model : _models)
      nb_triangle += model.triangle_count();

    std::vector<TriangleIndices> triangles;
    triangles.reserve(nb_triangle);
    for (const auto &model : _models) {
      model.write_indices_into_vec(triangles);
    }

    Buffer<> ibuff(
        ctx, nb_triangle * sizeof(TriangleIndices),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VMA_MEMORY_USAGE_CPU_TO_GPU);

    ibuff.write(0, nb_triangle * sizeof(TriangleIndices),
                reinterpret_cast<const uint8_t *>(triangles.data()));

    // Making BLASses
    std::vector<Blas> blas_vec;
    std::vector<Instance> instance_vec;
    blas_vec.reserve(_models.size());
    instance_vec.reserve(_models.size());

    size_t index_offset = 0;
    for (const Model &model : _models) {
      model.upload_meshes(ctx, vbuff, ibuff, index_offset, blas_vec,
                          instance_vec);
      index_offset += model.triangle_count();
    }

    Buffer<> instance_buffer(
        ctx, instance_vec.size() * sizeof(Instance),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    instance_buffer.write(
        0, instance_vec.size() * sizeof(Instance),
        reinterpret_cast<const uint8_t *>(instance_vec.data()));

    // add lights to the Scene
    // TODO add mesh lights
    std::vector<Buffer<>> primitives;
    ASSERT_ERR(!_punctualLights.empty(),
               "Punctual lights should not be empty !");
    primitives.emplace_back(ctx, _punctualLights.size() * sizeof(Light),
                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                            VMA_MEMORY_USAGE_CPU_TO_GPU);

    Buffer<> &light_buffer = primitives.back();

    light_buffer.write(
        0, _punctualLights.size() * sizeof(Light),
        reinterpret_cast<const uint8_t *>(_punctualLights.data()));

    size_t lights_index = blas_vec.size();
    size_t nb_of_light = 0;
    std::array<VkAabbPositionsKHR, 1> bbox_array;
    for (const Light &light : _punctualLights) {
      auto bbox = light.get_bbox();
      if (bbox.has_value()) {
        bbox_array[0] = light.get_bbox()->to_vk();
        blas_vec.emplace_back(
            Blas(ctx, bbox_array, nb_of_light));
      }
      nb_of_light++;
    }

    instance_buffer.write_into_descriptor(writter, Instance::BINDING,
                                          instance_buffer._count, 0,
                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vbuff.write_into_descriptor(writter, Vertex::BINDING, vbuff._count, 0,
                                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    ibuff.write_into_descriptor(writter, TriangleIndices::BINDING, ibuff._count,
                                0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    light_buffer.write_into_descriptor(writter, Light::BINDING,
                                       nb_of_light * sizeof(Light), 0,
                                       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    std::array<std::span<Blas>, 1> scene_blas_span_tab{
        std::span<Blas>{blas_vec}};
    std::array<std::span<Blas>, 1> light_blas_span{
        std::span<Blas>{blas_vec.begin() + lights_index, blas_vec.end()}};

    return UploadedAccStruct{
        .instance_buffer = {std::move(instance_buffer)},
        .primitive_buffers = std::move(primitives),
        .vertex_buffer = {std::move(vbuff)},
        .index_buffer = {std::move(ibuff)},
        .blas = std::move(blas_vec),
        .scene = Tlas(ctx, scene_blas_span_tab),
        .lights_scene = Tlas(ctx, light_blas_span),
    };
  }
};