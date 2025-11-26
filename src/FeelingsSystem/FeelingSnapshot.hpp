//
// Created by Mohamad on 26/11/2025.
//

#ifndef GL2D_FEELINGSNAPSHOT_HPP
#define GL2D_FEELINGSNAPSHOT_HPP
#include <string>
#include <optional>
#include "glm/glm.hpp"
namespace FeelingsSystem{
    struct FeelingSnapshot{
        std::string id{};

        float blendInMs{500.f};
        float blendOutMs{500.f};

        //Rendering
        std::optional<glm::vec4> colorGrade{};
        std::optional<float> vignette{};
        std::optional<float> bloomStrength{};
        std::optional<std::string> paletteID{};

        //Camera
        std::optional<float> zoomMul{};
        std::optional<glm::vec2> offset{};
        std::optional<float> shakeMagnitude{};
        std::optional<float> shakeRoughness{};
        std::optional<glm::vec4> parallaxBias{};
        std::optional<float> followSpeedMul{};

        //Gameplay
        std::optional<float> timeScale{};
        std::optional<float> entitySpeedMul{};
        std::optional<float> animationSpeedMul{};
        std::optional<float> accelerationSpeedMul{};
        std::optional<float> damageMul{};
        std::optional<float> armorMul{};



        //Particles
        std::optional<std::string > particlePresetId{};
        std::optional<glm::vec4> ambientLight{};
        std::optional<glm::vec4> fogColor{};
        std::optional<float> fogDensity{};

        //Audio
        std::optional<std::string > musicTrackId{};
        std::optional<float> musicVolume{};
        std::optional<std::string > sfxTag{};
        std::optional<float> sfxVolumeMul{};
        std::optional<std::string> audioFxPreset{};   // Named preset in your audio layer.
        std::optional<float> reverbSend{};            // 0..1 send amount.
        std::optional<float> delaySend{};             // 0..1 send amount.
        std::optional<float> delayTimeMs{};           // Echo/delay time override.
        std::optional<float> delayFeedback{};         // 0..1 feedback.
        std::optional<float> lowpassHz{};             // Optional low-pass cutoff.
        std::optional<float> highpassHz{};            // Optional high-pass cutoff.

        //UI
        std::optional<glm::vec4 > uiTint{};
        std::optional<float> uiLerpSpeed{};
    };
}
#endif //GL2D_FEELINGSNAPSHOT_HPP
