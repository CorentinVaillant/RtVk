#pragma once

#include "graphics/Blas.h"
#include "graphics/Tlas.h"
#include "hittables/Hittable.h"
#include "hittables/TriangleRef.h"
#include "renderer/renderer_utils.h"
#include <fastgltf/core.hpp>
#include <volk.h>

class Mesh : public IHittable {
  // -- Attributs
private:
  std::span<const Vertex> _vertices;
  std::vector<TriangleIndices> _indices;
  size_t _material_index;

  BBox bbox = BBOX_EMPTY;

  // -- Constructors
public:
  Mesh(fastgltf::Asset &asset, fastgltf::Mesh &mesh,
       std::span<const Vertex> vertex_buffer, size_t vertex_offset);

  // -- Methods
public:
  size_t triangle_count() const { return _indices.size(); }

  void write_indices_into_vec(std::vector<TriangleIndices> &out) const {
    for (const auto &tri : _indices) {
      out.push_back(tri);
    }
  }

  Blas upload_to_mesh_blas(VulkanContext &ctx, const Buffer<> &vbuff,
                           const Buffer<> &ibuff, glm::mat4 transform,
                           size_t instance_id, size_t index_offset) const {
    return Blas(ctx, _indices, vbuff, ibuff, instance_id, index_offset,
                transform);
  }

  Instance get_instance(uint32_t index_offset) const {
    return Instance{
        ._materialId = static_cast<uint32_t>(_material_index),
        ._indexOffset = index_offset,
    };
  }

  // -- IHittable impl
public:
  bool hit(Ray r, Interval ray_t, HitRecord *records) const override;
  BBox get_bbox() const override;
  HittableInfo gpu_info() const override;
};