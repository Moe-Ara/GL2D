//
// Created by Mohamad on 04/02/2025.
//

#ifndef GL2D_SHADER_HPP
#define GL2D_SHADER_HPP

#include <string>
#include <unordered_map>
#include <Gl/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
namespace Graphics {

    class Shader {
    public:
        Shader(const std::string& vertexPath, const std::string& fragmentPath);

        virtual ~Shader();

        Shader(const Shader &other) = delete;

        Shader &operator=(const Shader &other) = delete;

        Shader(Shader &&other) = delete;

        Shader &operator=(Shader &&other) = delete;
        // Enable and disable the shader program
        void enable() const;

        // Set uniform variables
        void setUniformFloat1(const std::string& name, float value);
        void setUniformInt1(const std::string& name, int value);
        void setUniformFloat2(const std::string& name, const glm::vec2& vector2);
        void setUniformFloat3(const std::string& name, const glm::vec3& vector3);

        void setUniformFloat4(const std::string& name, const glm::vec4& vector4);
        void setUniformMat4(const std::string& name, const glm::mat4& matrix);
        void setUniformMat3(const std::string& name, const glm::mat3& matrix);

    private:
        // Shader program ID
        GLuint m_shaderID;
        // File paths for the vertex and fragment shaders
        std::string m_vert_shader_path;
        std::string m_frag_shader_path;

        // Load and compile shaders, and link the shader program
        void load();
        bool compileShader(GLuint shader, const std::string& code);
        GLint getUniformLocation(const GLchar* name) const;
        void linkProgram(GLuint vertex, GLuint fragment);
        // Check for compilation errors
        bool check_compilation_errors(GLuint shader, const std::string& code);
    };

} // Graphics

#endif //GL2D_SHADER_HPP
