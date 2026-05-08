#pragma once

#include "fastgltf/types.hpp"
#include "graphics/Buffer.h"
#include "graphics/vulkan_context.h"
#include "hittables/Light.h"
#include "hittables/Mesh.h"
#include "hittables/TriangleRef.h"
#include "renderer/renderer_utils.h"
#include "types.h"
#include <glm/gtc/quaternion.hpp>

static inline glm::mat4
gltf_transform_to_glm(const std::variant<fastgltf::TRS, fastgltf::math::fmat4x4>
                          &gltf_transform) {
  auto result = std::visit(
      fastgltf::visitor{
          [](const fastgltf::TRS &trs) -> glm::mat4 {
            glm::vec3 translation(trs.translation[0], trs.translation[1],
                                  trs.translation[2]);
            glm::quat rotation(
                trs.rotation[3], trs.rotation[0], trs.rotation[1],
                trs.rotation[2]); // glm (w,x,y,z) -> fastgltf (x,y,z,w)
            glm::vec3 scale(trs.scale[0], trs.scale[1], trs.scale[2]);

            glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
            glm::mat4 R = glm::mat4_cast(rotation);
            glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

            return T * R * S;
          },
          [](const fastgltf::math::fmat4x4 &mat) -> glm::mat4 {
            glm::mat4 result;
            for (int col = 0; col < 4; ++col)
              for (int row = 0; row < 4; ++row)
                result[col][row] = mat.col(col)[row];
            return result;
          }},
      gltf_transform);

  return result;
}

class Model {
  struct ModelNode {
    std::optional<Mesh> mesh;
    glm::mat4 world_transform = glm::mat4(1);
    std::vector<size_t> childrens;
  };

  // -- Attributs --
private:
  std::vector<ModelNode> _nodes;
  size_t _nb_mesh = 0;
  BBox _bbox = BBOX_EMPTY;

  // -- Constructors --
public:
  Model(fastgltf::Asset &asset, fastgltf::Node &root,
        std::span<Vertex> vertices, std::vector<size_t> &mesh_vertex_offsets) {
    build_node(asset, root, vertices, glm::mat4(1), mesh_vertex_offsets);
  }

  // -- Methods --
public:
  size_t triangle_count() const {
    size_t result = 0;
    for (const auto &node : _nodes) {
      if (node.mesh.has_value())
        result += node.mesh->triangle_count();
    }
    return result;
  }

private:
  size_t build_node(fastgltf::Asset &asset, fastgltf::Node &node,
                    std::span<Vertex> vertices, glm::mat4 parent_transform,
                    std::vector<size_t> &mesh_vertex_offsets) {

    LOG(5, "Loading node : {}", node.name);
    glm::mat4 local_trans = gltf_transform_to_glm(node.transform);
    glm::mat4 world_trans = parent_transform * local_trans;

    ModelNode model_node;
    model_node.world_transform = world_trans;

    if (node.meshIndex.has_value()) {
      size_t offset = mesh_vertex_offsets[node.meshIndex.value()];
      model_node.mesh =
          Mesh(asset, asset.meshes[node.meshIndex.value()], vertices, offset);
      _bbox = BBox(_bbox, model_node.mesh.value().get_bbox());
      _nb_mesh++;
    }

    size_t node_index = _nodes.size();
    _nodes.emplace_back(std::move(model_node));

    for (size_t child_idx : node.children) {
      size_t child_node_index =
          build_node(asset, asset.nodes[child_idx], vertices, world_trans,
                     mesh_vertex_offsets);
      _nodes[node_index].childrens.push_back(child_node_index);
    }

    return node_index;
  }

  // -- IHitable impl --
public:
  bool hit(Ray r, Interval ray_t, HitRecord *records,
           std::span<const Vertex> vertex_buffer) const {
    bool hit_anything = false;

    for (const ModelNode &node : _nodes) {
      if (!node.mesh.has_value())
        continue;
      const Mesh &mesh = node.mesh.value();
      if (mesh.get_bbox().hit(r, ray_t) &&
          mesh.hit(r, ray_t, records, vertex_buffer)) {
        ray_t.max = records->t;
        hit_anything = true;
      }
    }

    return hit_anything;
  }

  BBox get_bbox() const { return _bbox; }

  void upload_meshes(VulkanContext &ctx, const Buffer<> &vbuff,
                     const Buffer<> &ibuff, size_t index_offset,
                     std::span<const Material> materials,
                     std::span<const Vertex> vertices,
                     std::vector<Blas> &blas_out,
                     std::vector<Instance> &instance_out,
                     std::vector<LightMesh> &emissive_out,
                     std::vector<size_t> &emissive_blas_indices_out) const {

    size_t mesh_offset = index_offset;
    for (const ModelNode &node : _nodes) {
      if (node.mesh.has_value()) {
        const Mesh &mesh = node.mesh.value();
        instance_out.push_back(mesh.get_instance(mesh_offset));
        blas_out.emplace_back(
            mesh.upload_to_mesh_blas(ctx, vbuff, ibuff, node.world_transform,
                                     instance_out.size() - 1, mesh_offset));

        glm::vec4 emission = mesh.emission(materials);
        const float MIN_SQ_EMISSION = 1;
        if (lenght_sq(emission) >= MIN_SQ_EMISSION) {
          emissive_out.emplace_back(LightMesh{
              .emission = emission,
              .area = mesh.area(vertices.data()),
              .start_index = static_cast<uint32_t>(mesh_offset),
              .end_index =
                  static_cast<uint32_t>(mesh_offset + mesh.triangle_count()),
          });

          emissive_blas_indices_out.push_back(blas_out.size() - 1);
        }

        mesh_offset += mesh.triangle_count();
      }
    }
  }

  void write_indices_into_vec(std::vector<TriangleIndices> &out) const {
    for (const auto &node : _nodes)
      if (node.mesh.has_value())
        node.mesh->write_indices_into_vec(out);
  }
};
