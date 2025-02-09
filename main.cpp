#include <iostream>
#include <filesystem>
#include <vector>
#include <map>
#include <memory>
#include <regex>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "GameObjects/Sprite.hpp"
#include "Managers/TextureManager.hpp"
#include "Managers/Animator.hpp"
#include "Graphics/Animation.hpp"
#include "Graphics/AnimationState.hpp"
#include "Managers/AnimationStateMachine.hpp"
#include "Graphics/Shader.hpp"
#include "Graphics/Frame.hpp"

namespace fs = std::filesystem;

std::map<std::string, std::shared_ptr<Graphics::Animation>> loadAnimationsFromFolder(const std::string& folderPath, float frameDuration) {
    std::map<std::string, std::shared_ptr<Graphics::Animation>> animations;
    std::regex pattern("(\\w+(?:_\\w+)*)_(\\d{4})\\.png");

    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.path().extension() == ".png") {
            std::string filename = entry.path().filename().string();
            std::smatch match;

            if (std::regex_match(filename, match, pattern)) {
                std::string animName = match[1];
                int frameIndex = std::stoi(match[2]);

                auto texture = Managers::TextureManager::loadTexture(entry.path().string());

                if (animations.find(animName) == animations.end()) {
                    animations[animName] = std::make_shared<Graphics::Animation>(1, 1, frameDuration);
                }

                animations[animName]->addFrame(Graphics::Frame{0, frameIndex}, texture);
            }
        }
    }

    return animations;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Character Animation", nullptr, nullptr);
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    auto shader = std::make_shared<Graphics::Shader>("shaders/vertex.vert", "shaders/fragment.frag");
    glm::mat4 projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f);

    auto animations = loadAnimationsFromFolder("assets/character", 0.15f);
    auto sprite = std::make_shared<GameObjects::Sprite>(nullptr, glm::vec2(400, 300), glm::vec2(128, 256));
    auto animator = std::make_shared<Managers::Animator>(sprite);
    auto stateMachine = std::make_shared<Managers::AnimationStateMachine>();

    // Create Animation States
    bool initialStateSet = false;
    for (const auto& [name, animation] : animations) {
        auto state = std::make_shared<Graphics::AnimationState>(animation, animator);
        stateMachine->addState(state);

        if (name.find("Idle") != std::string::npos && !initialStateSet) {
            stateMachine->setInitialState(state);
            initialStateSet = true;
        }
    }

    if (!initialStateSet && !animations.empty()) {
        stateMachine->setInitialState(std::make_shared<Graphics::AnimationState>(animations.begin()->second, animator));
    }

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        glClear(GL_COLOR_BUFFER_BIT);
        shader->enable();
        shader->setUniformMat4("projection", projection);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(sprite->getPosition(), 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

        shader->setUniformMat4("transform", model);
        glActiveTexture(GL_TEXTURE0);

        // Bind the current frame's texture
        if (auto currentAnimation = animator->getCurrentAnimation()) {
            auto currentFrame = currentAnimation->getCurrentFrame();
            currentFrame.texture->bind();
        }

        shader->setUniformInt1("spriteTexture", 0);

        stateMachine->update(deltaTime);
        sprite->draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
