#pragma once

#include "Camera.h"
#include "graphics/Buffer.h"
#include "graphics/vulkan_context.h"
#include "hittables/Hittable.h"
#include "types.h"

struct Scene {
  // - Backgrouds
  // - Materials

  Camera camera;
  std::unique_ptr<IAccStruct> _accStruct;
};

class GPUScene {
public:
  GPUScene(VulkanContext &ctx, const Scene &scene) {}
  // -- Attribts

public:
//TODO
private:
  VkDevice _ctxDevice;
};
