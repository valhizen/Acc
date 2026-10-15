#pragma once

#include "Engine/Engine.hpp"

class PlayerTag : public Component {
private:
  std::string name;

public:
  explicit PlayerTag(std::string playerName)
      : Component("PlayerTag"), name(playerName) {}

  const std::string &GetName() const { return name; }
  void SetName(std::string playerName) { name = playerName; }
};
