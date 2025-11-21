#include "GameObjects/Sprite.hpp"
#include "Graphics/Animation/Animation.hpp"
#include "Graphics/Animation/AnimationState.hpp"
#include "Graphics/Animation/AnimationStateMachine.hpp"
#include "Graphics/Animation/Animator.hpp"
#include "Graphics/Animation/Frame.hpp"
#include "Graphics/Animation/Loaders/AnimationMetadataLoader.hpp"
#include "Managers/TextureManager.hpp"
#include "RenderingSystem/Renderer.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

struct AnimationLoadResult {
  std::map<std::string, std::shared_ptr<Graphics::Animation>> animations;
  std::string initialState;
};

AnimationLoadResult
loadAnimationsFromMetadata(const std::string &metadataPath) {
  AnimationLoadResult result{};
  auto metadata = Loaders::AnimationMetadataLoader::loadFromFile(metadataPath);
  result.initialState = metadata.initialState;
  std::unordered_map<std::string, std::shared_ptr<GameObjects::Texture>>
      textureCache;
  auto fetchTexture =
      [&](const std::string &path) -> std::shared_ptr<GameObjects::Texture> {
    if (path.empty())
      return nullptr;
    auto it = textureCache.find(path);
    if (it != textureCache.end()) {
      return it->second;
    }
    auto texture = Managers::TextureManager::loadTexture(path);
    textureCache[path] = texture;
    return texture;
  };

  auto atlasTexture = fetchTexture(metadata.atlas.texturePath);

  for (const auto &entry : metadata.animations) {
    if (entry.frames.empty()) {
      continue;
    }

    const float animationFrameDuration = entry.defaultFrameDuration > 0.0f
                                             ? entry.defaultFrameDuration
                                             : metadata.defaultFrameDuration;

    auto animation = std::make_shared<Graphics::Animation>(
        metadata.atlas.rows, metadata.atlas.cols, animationFrameDuration,
        entry.loop, entry.playbackMode);
    animation->setName(entry.name);
    animation->setSharedTexture(atlasTexture);
    animation->setFrameDuration(animationFrameDuration);
    for (const auto &transition : entry.transitions) {
      animation->addTransition(Graphics::AnimationTransition{
          transition.target, transition.condition});
    }

    for (const auto &frameMeta : entry.frames) {
      Graphics::Frame frame{};
      frame.row = frameMeta.row;
      frame.column = frameMeta.column;
      frame.useCustomUV = frameMeta.useCustomUV;
      frame.uvRect = frameMeta.uvRect;
      frame.duration = frameMeta.duration > 0.0f ? frameMeta.duration
                                                 : animationFrameDuration;
      frame.eventName = frameMeta.eventName;

      if (!frameMeta.texturePath.empty()) {
        frame.texture = fetchTexture(frameMeta.texturePath);
      } else if (atlasTexture) {
        frame.texture = atlasTexture;
      } else {
        frame.texture = nullptr;
      }

      if (!frame.texture) {
        std::cerr << "Warning: Frame in animation '" << entry.name
                  << "' is missing a texture source." << std::endl;
      }

      animation->addFrame(frame);
    }
    result.animations[entry.name] = animation;
  }

  return result;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

int main() {
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW!" << std::endl;
    return -1;
  }

  GLFWwindow *window =
      glfwCreateWindow(800, 600, "Character Animation", nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to create GLFW window!" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW!" << std::endl;
    return -1;
  }

  Rendering::Renderer renderer("Shaders/vertex.vert", "Shaders/fragment.frag");
  glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);

  auto animationResult =
      loadAnimationsFromMetadata("assets/character/animations.json");
  const auto &animations = animationResult.animations;
  auto sprite = std::make_shared<GameObjects::Sprite>(
      nullptr, glm::vec2(400, 300), glm::vec2(128, 256));
  auto animator = std::make_shared<Graphics::Animator>(sprite);
  auto stateMachine = std::make_shared<Graphics::AnimationStateMachine>();

  // Create Animation States
  bool initialStateSet = false;
  std::unordered_map<std::string, std::shared_ptr<Graphics::AnimationState>>
      stateLookup;
  for (const auto &[name, animation] : animations) {
    auto state =
        std::make_shared<Graphics::AnimationState>(animation, animator);
    stateMachine->addState(state);
    stateLookup[name] = state;

    if (!initialStateSet) {
      if (!animationResult.initialState.empty() &&
          name == animationResult.initialState) {
        stateMachine->setInitialState(state);
        initialStateSet = true;
      } else if (animationResult.initialState.empty() &&
                 name.find("Idle") != std::string::npos) {
        stateMachine->setInitialState(state);
        initialStateSet = true;
      }
    }
  }

  if (!initialStateSet && !animations.empty()) {
    stateMachine->setInitialState(stateLookup.begin()->second);
  }

  std::unordered_map<std::string, std::shared_ptr<bool>> transitionSignals;
  auto ensureSignal = [&](const std::string &key) -> std::shared_ptr<bool> {
    if (key.empty())
      return nullptr;
    auto it = transitionSignals.find(key);
    if (it == transitionSignals.end()) {
      it = transitionSignals.emplace(key, std::make_shared<bool>(false)).first;
    }
    return it->second;
  };

  auto conditionFactory = [&](const std::string &key) {
    auto signal = ensureSignal(key);
    if (!signal) {
      return std::function<bool()>([] { return false; });
    }
    return std::function<bool()>([signal]() {
      if (*signal) {
        *signal = false;
        return true;
      }
      return false;
    });
  };

  for (const auto &pair : stateLookup) {
    const auto &state = pair.second;
    if (!state)
      continue;
    auto animation = state->getAnimation();
    if (!animation)
      continue;
    for (const auto &transition : animation->getTransitions()) {
      auto targetIt = stateLookup.find(transition.target);
      if (targetIt == stateLookup.end()) {
        std::cerr << "Warning: Transition target '" << transition.target
                  << "' not found." << std::endl;
        continue;
      }
      state->addTransition(targetIt->second,
                           conditionFactory(transition.condition));
    }
  }

  auto blinkTriggerSignal = ensureSignal("triggerBlink");

  animator->setFrameEventCallback(
      [](const std::string &eventName,
         const std::shared_ptr<Graphics::Animation> &animationPtr,
         size_t frameIndex) {
        if (eventName.empty()) {
          return;
        }
        std::cout << "[AnimationEvent] " << eventName << " frame "
                  << frameIndex;
        if (animationPtr) {
          std::cout << " (" << animationPtr->getName() << ")";
        }
        std::cout << std::endl;
      });

  double blinkAccumulator = 0.0;
  constexpr double blinkInterval = 4.5;

  double lastTime = glfwGetTime();
  while (!glfwWindowShouldClose(window)) {
    double currentTime = glfwGetTime();
    float deltaTime = static_cast<float>(currentTime - lastTime);
    lastTime = currentTime;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(sprite->getPosition(), 0.0f));

    blinkAccumulator += deltaTime;
    auto currentAnimation = animator->getCurrentAnimation();
    if (blinkTriggerSignal && blinkAccumulator >= blinkInterval) {
      if (currentAnimation && currentAnimation->getName() == "SlimeIdle") {
        blinkAccumulator = 0.0;
        *blinkTriggerSignal = true;
      }
    }

    stateMachine->update(deltaTime);
    renderer.beginFrame(projection, {0.2f, 0.3f, 0.3f, 1.0f});
    renderer.submitSprite(*sprite, model);
    renderer.endFrame();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
