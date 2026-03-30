#pragma once

#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "graphics/vma_usage.h"
#include "hittables/Hittable.h"
#include "hittables/Model.h"
#include "hittables/TriangleRef.h"
#include "renderer/renderer_utils.h"

class SceneCollection : public IAccStruct {
  // -- Attributs --
private:
  std::vector<Vertex> _vertices;
  HittableVector<Model> _models;
  std::vector<size_t> _mesh_vertex_offsets = {};

  // -- Constructors --
public:
  SceneCollection(fastgltf::Asset &assets, fastgltf::Scene &scene) {
    LOG(5, "Loading Scene {}", scene.name);
    load_vertices(assets);
    load_models(assets, scene);
  }

  SceneCollection(SceneCollection &&rval)
      : _vertices(std::move(rval._vertices)), _models(std::move(rval._models)),
        _mesh_vertex_offsets(std::move(rval._mesh_vertex_offsets)) {}

  SceneCollection &operator=(SceneCollection &&rval) {
    if (this != &rval) {
      this->_vertices = std::move(rval._vertices);
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
            auto uv_it  = primitive.findAttribute("TEXCOORD_0");

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

  // -- IAccStruct impl
public:
  uint32_t hit(Ray r, Interval ray_t, HitRecord *records) const override {
    return _models.hit(r, ray_t, records);
  }
  std::optional<const IHittable *> get_hitted(uint32_t index) const override {
    return _models.get_hitted(index);
  }

  // vulkan
  UploadedAccStruct upload_to_gpu(VulkanContext &ctx,
                                  DescriptorWriter &writter) const override {

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
    for (const auto &model : _models.vec())
      nb_triangle += model.triangle_count();

    std::vector<TriangleIndices> triangles;
    triangles.reserve(nb_triangle);
    for (const auto &model : _models.vec()) {
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
    for (const Model &model : _models.vec()) {
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
        instance_vec.size() * sizeof(Instance),
        reinterpret_cast<const uint8_t *>(instance_vec.data()));

    instance_buffer.write_into_descriptor(writter, Instance::BINDING,
                                          instance_buffer._count, 0,
                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    vbuff.write_into_descriptor(writter, Vertex::BINDING, vbuff._count, 0,
                                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    ibuff.write_into_descriptor(writter, TriangleIndices::BINDING, ibuff._count,
                                0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    return UploadedAccStruct{.instance_buffer = {std::move(instance_buffer)},
                             .primitive_buffers = {},
                             .vertex_buffer = {std::move(vbuff)},
                             .index_buffer = {std::move(ibuff)},
                             .tlas = Tlas(ctx, std::move(blas_vec))};
  }

  ~SceneCollection() = default;
};