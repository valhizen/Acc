#pragma once

#include "MeshComponent.hpp"
#include "Renderer.hpp"
#include <string>
#include <vector>

class Model {

public:
private:
  std::string name;
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

class ModelLoader {

public:
  /**
   *@brief Default Constructor
   */
  ModelLoader() = default;
  explicit ModelLoader(SimpleEngineVK::Renderer *_renderer) {
    if (!Initialize(_renderer)) {
      throw std::runtime_error("Failed to Initalize Model Loader");
    }
  }

  ~ModelLoader();

  Model *LoadGLTF(const std::string &filename);
  Model *GetModel(const std::string &name);

private:
  bool Initialize(SimpleEngineVK::Renderer *_renderer);

  SimpleEngineVK::Renderer *renderer = nullptr;

  bool ParseModel(const std::string &filename, Model* model);  
};
