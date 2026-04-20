#pragma once

#include "Camera.h"
#include "hittables/Light.h"
#include "hittables/SceneCollection.h"
#include "renderer/renderer_utils.h"
#include <cstdlib>
#include <filesystem>

struct Scene {

  std::vector<Camera> _camera;
  size_t _active_camera;
  SceneCollection _collection;
  std::vector<Material> _materials = {DEFAULT_MATERIAL};
  std::vector<Light> _punctualLights;
  float _time;

  static std::vector<Scene> load_from_gltf(std::filesystem::path gltf_path);
};
