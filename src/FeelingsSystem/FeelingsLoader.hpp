//
// Data-driven loader for feelings definitions.
//

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "FeelingSnapshot.hpp"

namespace FeelingsSystem {

class FeelingsLoader {
public:
    // Load feelings from a JSON file. Accepts either an array of feelings or
    // an object with a "feelings" array.
    static std::vector<FeelingSnapshot> loadList(const std::string& path);

    // Convenience: load and return a map keyed by id.
    static std::unordered_map<std::string, FeelingSnapshot> loadMap(const std::string& path);
};

} // namespace FeelingsSystem
