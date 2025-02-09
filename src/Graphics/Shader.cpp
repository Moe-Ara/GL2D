#include "Shader.hpp"
#include "Exceptions/ShaderException.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

namespace Graphics {

    Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath)
            : m_vert_shader_path(vertexPath), m_frag_shader_path(fragmentPath), m_shaderID(0)
    {
        load();
    }

    Shader::~Shader() {
        if (m_shaderID)
            glDeleteProgram(m_shaderID);
    }

    void Shader::enable() const {
        glUseProgram(m_shaderID);
    }

    void Shader::setUniformFloat1(const std::string &name, float value) {
        glUniform1f(getUniformLocation(name.c_str()), value);
    }

    void Shader::setUniformInt1(const std::string &name, int value) {
        glUniform1i(getUniformLocation(name.c_str()), value);
    }

    void Shader::setUniformFloat2(const std::string &name, const glm::vec2 &vector2) {
        glUniform2fv(getUniformLocation(name.c_str()), 1, glm::value_ptr(vector2));
    }

    void Shader::setUniformFloat4(const std::string &name, const glm::vec4 &vector4) {
        glUniform4fv(getUniformLocation(name.c_str()), 1, glm::value_ptr(vector4));
    }

    void Shader::setUniformMat4(const std::string &name, const glm::mat4 &matrix) {
        glUniformMatrix4fv(getUniformLocation(name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformMat3(const std::string &name, const glm::mat3 &matrix) {
        glUniformMatrix3fv(getUniformLocation(name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformFloat3(const std::string &name, const glm::vec3 &vector3) {
        glUniform3fv(getUniformLocation(name.c_str()), 1, glm::value_ptr(vector3));
    }

    void Shader::load() {
        std::string vertexCode, fragmentCode;
        try {
            std::ifstream vertexFile(m_vert_shader_path);
            std::ifstream fragmentFile(m_frag_shader_path);
            std::stringstream vShaderStream, fShaderStream;
            vShaderStream << vertexFile.rdbuf();
            fShaderStream << fragmentFile.rdbuf();
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        } catch (std::ifstream::failure &e) {
            throw ShaderException("Couldn't read shader files: " + std::string(e.what()));
        }
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        if (!compileShader(vertexShader, vertexCode) || !compileShader(fragmentShader, fragmentCode))
            throw ShaderException("Couldn't compile shaders");
        linkProgram(vertexShader, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    bool Shader::compileShader(GLuint shader, const std::string& code) {
        const char* source = code.c_str();
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        return check_compilation_errors(shader, code);
    }

    GLint Shader::getUniformLocation(const GLchar *name) const {
        GLint location = glGetUniformLocation(m_shaderID, name);
        if (location == -1)
            std::cerr << "Warning: Uniform '" << name << "' does not exist or is optimized out.\n";
        return location;
    }

    void Shader::linkProgram(GLuint vertex, GLuint fragment) {
        m_shaderID = glCreateProgram();
        glAttachShader(m_shaderID, vertex);
        glAttachShader(m_shaderID, fragment);
        glLinkProgram(m_shaderID);
        GLint success;
        glGetProgramiv(m_shaderID, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(m_shaderID, 512, nullptr, infoLog);
            std::cerr << "Shader Linking Error:\n" << infoLog << std::endl;
            throw ShaderException("Shader Linking Error");
        }
    }

    bool Shader::check_compilation_errors(GLuint shader, const std::string& code) {
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader Compilation Error:\n" << infoLog << std::endl;
            std::cerr << "Shader Source:\n" << code << std::endl;
            return false;
        }
        return true;
    }

} // namespace Graphics
