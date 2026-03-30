#pragma once

#include "glm/ext/matrix_float4x4.hpp"
#include "graphics/Buffer.h"
#include "graphics/GPUAccelerationStruct.h"
#include "graphics/utils.h"
#include "graphics/vma_usage.h"
#include "graphics/vulkan_context.h"

#include "renderer/renderer_utils.h"
#include "types.h"

#include <utility>

class IHittable {
public:
  virtual bool hit(Ray r, Interval ray_t, HitRecord *records) const = 0;
  virtual BBox get_bbox() const = 0;

  virtual HittableInfo gpu_info() const = 0;

  virtual Blas get_blas(VulkanContext &ctx) const {

    VkAabbPositionsKHR aabb = {get_bbox().to_vk()};
    return Blas(ctx, {&aabb, 1}, glm::mat4(1));
  }

  virtual ~IHittable() = default;
};

template <typename T>
concept Hittable = std::is_base_of_v<IHittable, T>;

struct UploadedAccStruct {
  std::optional<Buffer<>> instance_buffer;
  std::vector<Buffer<>> primitive_buffers;
  std::optional<Buffer<>> vertex_buffer;
  std::optional<Buffer<>> index_buffer;
  Tlas tlas;
};

class IAccStruct {
public:
  virtual ~IAccStruct() = default;

  static constexpr uint32_t MISS_INDEX = -1;
  virtual uint32_t hit(Ray r, Interval ray_t, HitRecord *records) const = 0;
  virtual std::optional<const IHittable *> get_hitted(uint32_t index) const = 0;

  // vulkan
  virtual UploadedAccStruct upload_to_gpu(VulkanContext &ctx,
                                          DescriptorWriter &writter) const = 0;
};

template <Hittable T> class HittableVector : public IAccStruct {
public:
  // -- Constructors
  HittableVector() = default;

  HittableVector(std::vector<T> &&objects) {
    for (auto &&object : std::move(objects)) {
      _objects.push_back(std::move(object));
    }
  }

  NO_COPY(HittableVector);

  ~HittableVector() {}

  // move constructors
  HittableVector(HittableVector &&other)
      : _objects(std::move(other._objects)) {}

  HittableVector &operator=(HittableVector &&other) {
    if (this != &other) {
      _objects = std::move(other._objects);
    }
    return *this;
  }

  // -- Methods
  void push(T &&object) { _objects.push_back(std::move(object)); }
  size_t size() const { return _objects.size(); }

  const std::vector<T> &vec() const { return _objects; }
  std::vector<T> &vec() { return _objects; }

  // -- IAccStruct impl
  uint32_t hit(Ray r, Interval ray_t, HitRecord *records) const override {
    float closest_so_far = ray_t.max;
    uint32_t closest_index = MISS_INDEX;
    HitRecord temp_rec;

    for (uint32_t i = 0; i < _objects.size(); i++) {
      const IHittable &object = _objects[i];
      if (object.hit(r, Interval(ray_t.min, closest_so_far), &temp_rec)) {
        closest_index = i;
        closest_so_far = temp_rec.t;
        *records = temp_rec;
      }
    }
    return closest_index;
  }
  std::optional<const IHittable *> get_hitted(uint32_t index) const override {
    return (index < _objects.size())
               ? std::optional<const IHittable *>{&_objects[index]}
               : std::nullopt;
  }

  UploadedAccStruct upload_to_gpu(VulkanContext &ctx,
                                  DescriptorWriter &writter) const override {
    std::vector<Blas> blas_vec;
    for (auto &obj : _objects)
      blas_vec.emplace_back(obj.get_blas(ctx));

    Tlas tlas(ctx, std::move(blas_vec));

    // Upload in buffer

    size_t total_size = 0;
    for (const T &obj : _objects) {
      total_size += obj.gpu_info().obj_size;
    }
    Buffer<> buffer(ctx, total_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VMA_MEMORY_USAGE_CPU_TO_GPU);

    std::vector<HittableInfo> infos;
    infos.reserve(_objects.size());
    // Write in descriptor
    struct BindingWritterInfo {
      uint32_t total_size;
    };
    std::unordered_map<uint32_t, BindingWritterInfo> binding_map;

    size_t current_offset = 0;
    for (const T &obj : _objects) {
      const HittableInfo info = obj.gpu_info();
      buffer.write(current_offset, info.obj_size, info.gpu_instance);
      infos.push_back(info);

      if (binding_map.contains(info.binding)) {
        binding_map[info.binding].total_size += info.obj_size;
      } else {
        binding_map[info.binding] = BindingWritterInfo{info.obj_size};
      }

      current_offset += info.obj_size;
    }

    for (const auto &key_val : binding_map) {
      const auto &[binding, info] = key_val;
      buffer.write_into_descriptor(writter, binding, info.total_size, 0,
                                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    }

    std::vector<Buffer<>> primitive_buffers;
    primitive_buffers.emplace_back(std::move(buffer));
    return UploadedAccStruct{
        .primitive_buffers = std::move(primitive_buffers),
        .tlas = std::move(tlas),
    };
  }

private:
  // -- Members
  std::vector<T> _objects;
};