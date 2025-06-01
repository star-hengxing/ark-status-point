#pragma once

#include "base/base.hpp"

NAMESPACE_BEGIN(ark)

enum class Status
{
    health         = 0,
    stamina        = 1,
    torpor         = 2,
    oxygen         = 3,
    food           = 4,
    water          = 5,
    temperature    = 6,
    weight         = 7,
    melee_damage   = 8,
    movement_speed = 9,
    fortitude      = 10,
    crafting_speed = 11,
    none,
};

struct Person
{
    u8 hp = 0;
    u8 stamina = 0;
    u8 oxygen = 0;
    u8 food = 0;
    u8 water = 0;
    u8 weight = 0;
    u8 melee = 0;
    u8 speed = 0;
    u8 fortitude = 0;
    u8 crafting = 0;
};

struct Dino
{
    u8 hp = 0;
    u8 stamina = 0;
    u8 oxygen = 0;
    u8 food = 0;
    u8 weight = 0;
    u8 melee = 0;
    u8 speed = 0;
};

bool is_running() noexcept;

NAMESPACE_END(ark)
