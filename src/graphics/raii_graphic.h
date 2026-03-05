#pragma once

#include "types.h"
#include <vulkan/vulkan_core.h>
#include <volk.h>

template <typename T, auto DestroyFun> class DestructorFromDevice {
public:
  DestructorFromDevice(VkDevice device) : _device(device) {}

  // -- IDestructor impl --

  void release(T value) noexcept { DestroyFun(_device, value); }
  T discard(T value) noexcept { return value; }

private:
  // -- attributs --
  VkDevice _device;
};

template <typename T, auto DestroyFun> class DestructorFromDevicePool {
public:
  DestructorFromDevicePool(VkDevice device, VkDescriptorPool pool)
      : _device(device), _pool(pool) {}

  // -- IDestructor impl --

  void release(T value) noexcept { DestroyFun(_device, _pool, value); }
  T discard(T value) noexcept { return value; }

private:
  // -- attributs --
  VkDevice _device;
  VkDescriptorPool _pool;
};

template <typename T, auto DestroyFun>
using DeviceScopeGuard = ScopeGuard<T, DestructorFromDevice<T, DestroyFun>>;

template <typename T, auto DestroyFun>
using DevicePoolScopeGuard =
    ScopeGuard<T, DestructorFromDevicePool<T, DestroyFun>>;

// -- Specialisations --

// Raii_VkImage

using Raii_VkImage =
    DeviceScopeGuard<VkImage, [](VkDevice device, VkImage image) {
      vkDestroyImage(device, image, nullptr);
    }>;

// Raii_VkDescriptorSet

using Raii_VkDescriptorSet =
    DevicePoolScopeGuard<VkDescriptorSet,
                         [](VkDevice device, VkDescriptorPool pool,
                            VkDescriptorSet descr_set) {
                           vkFreeDescriptorSets(device, pool, 1, &descr_set);
                         }>;
