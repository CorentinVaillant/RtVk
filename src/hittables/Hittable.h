#pragma once

#include "graphics/Buffer.h"
#include "graphics/GPUAccelerationStruct.h"
#include "graphics/vma_usage.h"
#include "graphics/vulkan_context.h"

#include "renderer/renderer_utils.h"
#include "types.h"

#include <utility>
#include <vulkan/vulkan_core.h>

struct HitRecord {
  glm::vec3 p;
  float t;
  glm::vec3 normal;
  glm::vec2 uv;
  bool front_face;

  void set_face_normal(glm::vec3 ray_dir, glm::vec3 out_normal) {
    front_face = glm::dot(ray_dir, out_normal) < 0;
    normal = front_face ? out_normal : -out_normal;
  }
};

class IHittable {
public:
  virtual bool hit(Ray r, Interval ray_t, HitRecord *records) const = 0;
  virtual BBox get_bbox() const = 0;

  /// Should return the size taken by the object inside a Vulkan Buffer.
  /// Should be identical for each objects of the same types
  virtual uint32_t get_in_buffer_size() const = 0;
  // Should return the next write address
  virtual uint32_t write_in_buffer(Buffer<uint8_t> &buffer,
                                   uint32_t index) const = 0;

  virtual ~IHittable() = default;
};

template <typename T>
concept Hittable = std::is_base_of_v<IHittable, T>;

class IAccStruct {
public:
  virtual ~IAccStruct() = default;

  static constexpr uint32_t MISS_INDEX = -1;
  virtual uint32_t hit(Ray r, Interval ray_t, HitRecord *records) const = 0;
  virtual std::optional<const IHittable *> get_hitted(uint32_t index) const = 0;

  // vulkan
  virtual Tlas get_gpu_struct(VulkanContext &ctx) const = 0;
  virtual std::vector<Buffer<>> upload_to_buffers(VulkanContext &ctx) const = 0;
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
  template <Hittable Hit> void push(Hit &&object) {
    _objects.push_back(std::make_unique(std::move(object)));
  }

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

  Tlas get_gpu_struct(VulkanContext &ctx) const override {
    std::vector<VkAabbPositionsKHR> aabbs;
    for (auto &obj : _objects)
      aabbs.push_back(obj.get_bbox().to_vk());

    std::vector<Blas> blas_vec;
    blas_vec.emplace_back(ctx, aabbs);

    return Tlas(ctx, std::move(blas_vec));
  }

  std::vector<Buffer<>> upload_to_buffers(VulkanContext &ctx) const override {
    std::vector<Buffer<>> result;
    if (_objects.empty()) {
      result.emplace_back(ctx, 0, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                          VMA_MEMORY_USAGE_CPU_TO_GPU);
      return result;
    }

    uint32_t object_size = _objects[0].get_in_buffer_size();
    Buffer<> result_buffer(ctx, object_size * _objects.size(),
                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           VMA_MEMORY_USAGE_CPU_TO_GPU);
    uint8_t curr_buffer_index = 0;

    for (const T &obj : _objects)
      curr_buffer_index = obj.write_in_buffer(result_buffer, curr_buffer_index);

    result.emplace_back(std::move(result_buffer));
    return result;
  }

private:
  // -- Members
  std::vector<T> _objects;
};