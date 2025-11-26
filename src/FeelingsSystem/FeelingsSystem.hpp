//
// Convenience alias so callers can refer to FeelingsSystem instead of FeelingsManager.
//

#ifndef GL2D_FEELINGSSYSTEM_ALIAS_HPP
#define GL2D_FEELINGSSYSTEM_ALIAS_HPP

#include "FeelingsManager.hpp"

namespace FeelingsSystem {
// Alias type to avoid breaking existing includes that expect FeelingsSystem.
using FeelingsSystem = FeelingsManager;
} // namespace FeelingsSystem

#endif // GL2D_FEELINGSSYSTEM_ALIAS_HPP
