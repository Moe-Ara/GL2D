#pragma once

#include "Physics/PhysicsUnits.hpp"

struct EntityAttributes {
  float moveSpeed = PhysicsUnits::toUnits(1.5f);
  float acceleration = PhysicsUnits::toUnits(20.0f);
  float deceleration = PhysicsUnits::toUnits(20.0f);

  float maxHealth = 100.0f;
  float healt = 100.0f;
  float attackDamage = 10.0f;

  float attackSpeed = 1.0f;
  float critChance = 0.0f;
};
