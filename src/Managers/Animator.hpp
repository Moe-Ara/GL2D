//
// Created by Mohamad on 08/02/2025.
//

#ifndef GL2D_ANIMATOR_HPP
#define GL2D_ANIMATOR_HPP

#include <memory>
#include "GameObjects/Sprite.hpp"
#include "Graphics/Animation.hpp"

namespace Managers {

    class Animator {
    public:
        Animator(std::shared_ptr<GameObjects::Sprite> sprite);

        virtual ~Animator();

        Animator(const Animator &other) = delete;

        Animator &operator=(const Animator &other) = delete;

        Animator(Animator &&other) = delete;

        Animator &operator=(Animator &&other) = delete;

        void play(const std::shared_ptr<Graphics::Animation> &animation);

        void stop();

        void pause();

        void resume();

        void update(float deltaTime);
        void reset();


    private:
        void updateSpriteUV();

        std::shared_ptr<GameObjects::Sprite> m_sprite;
        std::shared_ptr<Graphics::Animation> m_currentAnimation;
        size_t m_currentFrameIndex;
        float m_elapsedTime;
        bool m_isPlaying;
    };

} // Managers

#endif //GL2D_ANIMATOR_HPP
