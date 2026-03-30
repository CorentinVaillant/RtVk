#include "Mesh.h"
#include "fastgltf/tools.hpp"
#include "hittables/TriangleRef.h"
#include "renderer/renderer_utils.h"

Mesh::Mesh(fastgltf::Asset &asset, fastgltf::Mesh &mesh,
           std::span<const Vertex> vertex_buffer, size_t vertex_offset)
    : _vertices(vertex_buffer){

  LOG(5, "Loading mesh : {}", mesh.name);
  for (fastgltf::Primitive &primitive : mesh.primitives) {
    std::array<uint32_t, 3> indices;

    auto indice_acc_index_result = primitive.indicesAccessor;
    if (!indice_acc_index_result.has_value())
      continue;

    auto &indice_acc_index = indice_acc_index_result.value();
    auto &accessor = asset.accessors[indice_acc_index];

    fastgltf::iterateAccessorWithIndex<uint32_t>(
        asset, accessor, [vertex_offset,&indices, this](uint32_t index, size_t i) {
          indices[i % 3] = index + vertex_offset;

          if (i % 3 == 2) {
            _indices.emplace_back(indices[0], indices[1], indices[2]);
            bbox = BBox(bbox, _indices.back().get_bbox(_vertices.data()));
          }
        });

    if (primitive.materialIndex.has_value())
      _material_index = primitive.materialIndex.value() +1;
    else 
      _material_index = 0;
  }
}

// -- IHittable impl --

bool Mesh::hit(Ray r, Interval ray_t, HitRecord *records) const {
  bool hit_anything = false;

  for (const TriangleIndices &triangle : _indices) {
    if (triangle.hit(_vertices.data(), r, ray_t, records)) {
      ray_t.max = records->t;
      hit_anything = true;
    }
  }

  return hit_anything;
}

BBox Mesh::get_bbox() const { return bbox; }

HittableInfo Mesh::gpu_info() const {
  return HittableInfo{
      // TODO
  };
}
