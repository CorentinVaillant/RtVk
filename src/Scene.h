#pragma once

#include "Camera.h"
#include "hittables/Hittable.h"
#include "renderer/renderer_utils.h"

struct Scene {
  // - Backgrouds
  // - Materials

  Camera _camera;
  std::unique_ptr<IAccStruct> _accStruct;
  std::vector<Material> _materials = {DEFAULT_MATERIAL};
  float _time;
};
