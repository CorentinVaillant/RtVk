#pragma once

#include "Camera.h"
#include "graphics/Buffer.h"
#include "graphics/vulkan_context.h"
#include "hittables/Hittable.h"
#include "types.h"

struct Scene {
  // - Backgrouds
  // - Materials

  Camera _camera;
  std::unique_ptr<IAccStruct> _accStruct;
  float _time;
};
