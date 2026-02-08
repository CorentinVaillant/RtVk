#pragma once

#include "Camera.h"
#include "hittables/Hittable.h"
#include "types.h"

struct Scene {
  // - Backgrouds
  // - Materials

  Camera camera;
  std::unique_ptr<IAccStruct> _accStruct;
};