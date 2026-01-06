//
// Engine subsystem exceptions that wrap fatal errors so callers can present friendly diagnostics.
//

#ifndef GL2D_SUBSYSTEMEXCEPTIONS_HPP
#define GL2D_SUBSYSTEMEXCEPTIONS_HPP

#include "Gl2DException.hpp"
#include <string>

namespace Engine {

    class AudioException : public GL2DException {
    public:
        explicit AudioException(const std::string &msg)
                : GL2DException("AudioException: " + msg) {}
    };

    class InputException : public GL2DException {
    public:
        explicit InputException(const std::string &msg)
                : GL2DException("InputException: " + msg) {}
    };

    class PrefabException : public GL2DException {
    public:
        explicit PrefabException(const std::string &msg)
                : GL2DException("PrefabException: " + msg) {}
    };

    class UIException : public GL2DException {
    public:
        explicit UIException(const std::string &msg)
                : GL2DException("UIException: " + msg) {}
    };

    class LevelException : public GL2DException {
    public:
        explicit LevelException(const std::string &msg)
                : GL2DException("LevelException: " + msg) {}
    };

    class ParticleException : public GL2DException {
    public:
        explicit ParticleException(const std::string &msg)
                : GL2DException("ParticleException: " + msg) {}
    };

    class FeelingsException : public GL2DException {
    public:
        explicit FeelingsException(const std::string &msg)
                : GL2DException("FeelingsException: " + msg) {}
    };

    class ResourceException : public GL2DException {
    public:
        explicit ResourceException(const std::string &msg)
                : GL2DException("ResourceException: " + msg) {}
    };

} // namespace Engine

#endif //GL2D_SUBSYSTEMEXCEPTIONS_HPP
