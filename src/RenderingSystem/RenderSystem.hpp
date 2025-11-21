//
// Created by Codex on 23/11/2025.
//

#ifndef GL2D_RENDERSYSTEM_HPP
#define GL2D_RENDERSYSTEM_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include "Engine/Scene.hpp"
#include "Graphics/Camera/Camera.hpp"
#include "RenderingSystem/Renderer.hpp"

class RenderSystem {
public:
    RenderSystem()=delete;
    virtual ~RenderSystem()=delete;

    // Renders all sprites within the camera view, with padding of half the view size.
    static void renderScene(Scene& scene, Camera& camera, Rendering::Renderer& renderer);
};

#endif //GL2D_RENDERSYSTEM_HPP
