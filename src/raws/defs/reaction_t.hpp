#pragma once

#include "../../stdafx.h"
#include "../reaction_input.hpp"

constexpr int special_reaction_cooking = 0;
constexpr int special_reaction_tanning = 1;

struct reaction_t {
    std::string tag = "";
    std::string name = "";
    std::string workshop = "";
    std::vector<reaction_input_t> inputs;
    std::vector<std::pair<std::string, int>> outputs;
    std::string skill = "";
    int difficulty = 10;
    bool automatic = false;
    int power_drain = 0;
    bool emits_smoke = false;
    std::bitset<2> specials;
};
