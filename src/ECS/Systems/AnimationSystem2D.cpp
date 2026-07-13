#include "ECS/Systems/AnimationSystem2D.hpp"

#include "ECS/Components/Animation2D.hpp"
#include "ECS/Components/CharacterMotor.hpp"
#include "ECS/Components/Climbable2D.hpp"
#include "ECS/Components/SpriteRender.hpp"
#include "ECS/Registry.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ECS {
namespace {
bool transitionPasses(const CompiledAnimationTransition2D& transition,
                      const AnimationParameters2D& parameters) {
    switch (transition.condition) {
        case AnimationCondition2D::Always:
            return true;
        case AnimationCondition2D::BoolEquals: {
            const auto found = parameters.boolValues.find(transition.parameter);
            return found != parameters.boolValues.end() &&
                   found->second == transition.expectedBool;
        }
        case AnimationCondition2D::FloatGreater:
        case AnimationCondition2D::FloatLess:
        case AnimationCondition2D::FloatGreaterEqual:
        case AnimationCondition2D::FloatLessEqual: {
            const auto found = parameters.floatValues.find(transition.parameter);
            if (found == parameters.floatValues.end() || !std::isfinite(found->second)) {
                return false;
            }
            switch (transition.condition) {
                case AnimationCondition2D::FloatGreater:
                    return found->second > transition.threshold;
                case AnimationCondition2D::FloatLess:
                    return found->second < transition.threshold;
                case AnimationCondition2D::FloatGreaterEqual:
                    return found->second >= transition.threshold;
                case AnimationCondition2D::FloatLessEqual:
                    return found->second <= transition.threshold;
                default:
                    return false;
            }
        }
    }
    return false;
}

void pushEvent(AnimationEventQueue2D& queue, AnimationEventKind2D kind,
               std::string name, const std::string& state, std::size_t frameIndex) {
    queue.events.push_back({kind, std::move(name), state, frameIndex});
}

void applyFrame(const AnimationState2D& state, const Animator2D& animator,
                SpriteRender& renderable) {
    const AnimationFrame2D& frame = state.clip.frames[animator.frameIndex];
    renderable.useCustomUV = true;
    renderable.uvRect = frame.uvRect;
    renderable.animationTint = frame.tint;
    renderable.textureOverride = frame.texture;
    renderable.normalTextureOverride = frame.normalTexture;
}

void emitFrameEvent(const AnimationState2D& state, const Animator2D& animator,
                    AnimationEventQueue2D& events) {
    const AnimationFrame2D& frame = state.clip.frames[animator.frameIndex];
    if (!frame.event.empty()) {
        pushEvent(events, AnimationEventKind2D::Frame, frame.event,
                  state.name, animator.frameIndex);
    }
}

void enterState(Animator2D& animator, SpriteRender& renderable,
                AnimationEventQueue2D& events, std::size_t targetIndex,
                bool emitExit) {
    if (emitExit) {
        const AnimationState2D& previous = animator.graph->state(animator.stateIndex);
        pushEvent(events, AnimationEventKind2D::StateExited, previous.name,
                  previous.name, animator.frameIndex);
    }

    animator.stateIndex = targetIndex;
    const AnimationState2D& current = animator.graph->state(targetIndex);
    const bool reverse = current.clip.playback == AnimationPlayback2D::LoopReverse ||
                         current.clip.playback == AnimationPlayback2D::OnceReverse;
    animator.frameIndex = reverse ? current.clip.frames.size() - 1 : 0;
    animator.frameElapsedSeconds = 0.0f;
    animator.stateElapsedSeconds = 0.0f;
    animator.frameDirection = reverse ? -1 : 1;
    animator.playing = true;
    animator.completed = false;
    pushEvent(events, AnimationEventKind2D::StateEntered, current.name,
              current.name, animator.frameIndex);
    applyFrame(current, animator, renderable);
    emitFrameEvent(current, animator, events);
}

bool advanceFrame(Animator2D& animator, SpriteRender& renderable,
                  AnimationEventQueue2D& events) {
    const AnimationState2D& state = animator.graph->state(animator.stateIndex);
    const std::size_t frameCount = state.clip.frames.size();
    bool changed = false;
    switch (state.clip.playback) {
        case AnimationPlayback2D::Loop:
            ++animator.frameIndex;
            if (animator.frameIndex >= frameCount) {
                animator.frameIndex = 0;
                pushEvent(events, AnimationEventKind2D::ClipLooped, state.name,
                          state.name, animator.frameIndex);
            }
            changed = true;
            break;
        case AnimationPlayback2D::Once:
            if (animator.frameIndex + 1 < frameCount) {
                ++animator.frameIndex;
                changed = true;
            } else {
                animator.playing = false;
                animator.completed = true;
                pushEvent(events, AnimationEventKind2D::Completed, state.name,
                          state.name, animator.frameIndex);
            }
            break;
        case AnimationPlayback2D::LoopReverse:
            if (animator.frameIndex == 0) {
                animator.frameIndex = frameCount - 1;
                pushEvent(events, AnimationEventKind2D::ClipLooped, state.name,
                          state.name, animator.frameIndex);
            } else {
                --animator.frameIndex;
            }
            changed = true;
            break;
        case AnimationPlayback2D::OnceReverse:
            if (animator.frameIndex > 0) {
                --animator.frameIndex;
                changed = true;
            } else {
                animator.playing = false;
                animator.completed = true;
                pushEvent(events, AnimationEventKind2D::Completed, state.name,
                          state.name, animator.frameIndex);
            }
            break;
        case AnimationPlayback2D::PingPong:
            if (frameCount == 1) {
                pushEvent(events, AnimationEventKind2D::ClipLooped, state.name,
                          state.name, 0);
                break;
            }
            if (animator.frameDirection > 0) {
                if (animator.frameIndex + 1 >= frameCount) {
                    animator.frameDirection = -1;
                    animator.frameIndex = frameCount - 2;
                } else {
                    ++animator.frameIndex;
                }
            } else if (animator.frameIndex == 0) {
                animator.frameDirection = 1;
                animator.frameIndex = 1;
                pushEvent(events, AnimationEventKind2D::ClipLooped, state.name,
                          state.name, animator.frameIndex);
            } else {
                --animator.frameIndex;
            }
            changed = true;
            break;
    }

    if (changed) {
        applyFrame(state, animator, renderable);
        emitFrameEvent(state, animator, events);
    }
    return animator.playing;
}
}

void CharacterAnimationParameterSystem2D::update(Registry& registry) {
    registry.each<AnimationParameters2D, KinematicBody2D, GroundContact2D>(
        [&registry](Entity entity, AnimationParameters2D& parameters,
           const KinematicBody2D& body, const GroundContact2D& contact) {
            const auto* climbing = registry.tryGet<ClimbingState2D>(entity);
            const bool isClimbing = climbing && climbing->active;
            parameters.setFloat("speed", std::abs(body.velocity.x));
            parameters.setFloat("verticalVelocity", body.velocity.y);
            parameters.setBool("moving", std::abs(body.velocity.x) > 1.0f);
            parameters.setBool("grounded", contact.grounded);
            parameters.setBool("climbing", isClimbing);
            parameters.setBool("rising", !isClimbing && !contact.grounded &&
                                               body.velocity.y > 0.0f);
            parameters.setBool("falling", !isClimbing && !contact.grounded &&
                                                body.velocity.y <= 0.0f);
        });
}

void AnimationSystem2D::beginFrame(Registry& registry) {
    registry.each<AnimationEventQueue2D>(
        [](Entity, AnimationEventQueue2D& events) { events.events.clear(); });
}

void AnimationSystem2D::update(Registry& registry, float fixedDeltaTime,
                               float feelingSpeedMultiplier) {
    if (!std::isfinite(fixedDeltaTime) || fixedDeltaTime <= 0.0f ||
        !std::isfinite(feelingSpeedMultiplier) ||
        feelingSpeedMultiplier < 0.0f) {
        throw std::invalid_argument(
            "AnimationSystem2D delta must be positive/finite and feeling speed must be finite/non-negative");
    }

    registry.each<Animator2D, AnimationParameters2D, SpriteRender,
                  AnimationEventQueue2D>(
        [fixedDeltaTime, feelingSpeedMultiplier](Entity, Animator2D& animator,
                         const AnimationParameters2D& parameters,
                         SpriteRender& renderable, AnimationEventQueue2D& events) {
            if (!animator.graph) {
                return;
            }
            if (!std::isfinite(animator.playbackSpeed) || animator.playbackSpeed < 0.0f) {
                throw std::invalid_argument("Animator2D playback speed must be finite and non-negative");
            }

            const bool initialized = animator.stateIndex !=
                std::numeric_limits<std::size_t>::max();
            if (!initialized) {
                enterState(animator, renderable, events,
                           animator.graph->initialStateIndex(), false);
            }

            if (!animator.requestedState.empty()) {
                const auto requested = animator.graph->findState(animator.requestedState);
                if (!requested) {
                    throw std::invalid_argument(
                        "Animator2D requested unknown state: " + animator.requestedState);
                }
                if (*requested != animator.stateIndex) {
                    enterState(animator, renderable, events, *requested, true);
                }
                animator.requestedState.clear();
            }

            for (const CompiledAnimationTransition2D& transition : animator.graph->transitions()) {
                if ((!transition.anySource &&
                     transition.fromState != animator.stateIndex) ||
                    animator.stateElapsedSeconds < transition.minimumStateSeconds ||
                    !transitionPasses(transition, parameters)) {
                    continue;
                }
                const std::size_t target = transition.toState;
                if (target != animator.stateIndex) {
                    enterState(animator, renderable, events, target, true);
                    break;
                }
            }

            const float scaledDelta = fixedDeltaTime * animator.playbackSpeed *
                                      feelingSpeedMultiplier;
            animator.stateElapsedSeconds += scaledDelta;
            if (!animator.playing || scaledDelta <= 0.0f) {
                applyFrame(animator.graph->state(animator.stateIndex), animator, renderable);
                return;
            }

            animator.frameElapsedSeconds += scaledDelta;
            constexpr int maxFrameAdvances = 1024;
            for (int advances = 0; advances < maxFrameAdvances && animator.playing; ++advances) {
                const AnimationState2D& activeState = animator.graph->state(animator.stateIndex);
                const float duration = activeState.clip.frames[animator.frameIndex].durationSeconds;
                if (animator.frameElapsedSeconds < duration) {
                    break;
                }
                animator.frameElapsedSeconds -= duration;
                advanceFrame(animator, renderable, events);
            }
        });
}

} // namespace ECS
