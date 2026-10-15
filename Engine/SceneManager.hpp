#pragma once


enum class SceneType {
  UI,
  GamePlay
};


class SceneManager {
private:
public:
  void LoadScene();
  void UnloadScene();  
};


