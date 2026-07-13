#include "Shader.hpp"
#include "Exceptions/ShaderException.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>
#include <glm/gtc/type_ptr.hpp>

namespace Graphics {

    Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath)
            : m_shaderID(0), m_vert_shader_path(vertexPath), m_frag_shader_path(fragmentPath)
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

    void Shader::setUniformFloat1(const std::string &name, float value) const {
        glUniform1f(getUniformLocation(name.c_str()), value);
    }

    void Shader::setUniformInt1(const std::string &name, int value) const {
        glUniform1i(getUniformLocation(name.c_str()), value);
    }

    void Shader::setUniformFloat2(const std::string &name, const glm::vec2 &vector2) const {
        glUniform2fv(getUniformLocation(name.c_str()), 1, glm::value_ptr(vector2));
    }

    void Shader::setUniformFloat4(const std::string &name, const glm::vec4 &vector4) const {
        glUniform4fv(getUniformLocation(name.c_str()), 1, glm::value_ptr(vector4));
    }

    void Shader::setUniformMat4(const std::string &name, const glm::mat4 &matrix) const {
        glUniformMatrix4fv(getUniformLocation(name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformMat3(const std::string &name, const glm::mat3 &matrix) const {
        glUniformMatrix3fv(getUniformLocation(name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
    }

    void Shader::setUniformFloat3(const std::string &name, const glm::vec3 &vector3) const {
        glUniform3fv(getUniformLocation(name.c_str()), 1, glm::value_ptr(vector3));
    }

    void Shader::load() {
        const auto readSource = [](const std::string& path) {
            std::ifstream file(path);
            if (!file) {
                throw ShaderException("Unable to open shader file: " + path);
            }
            std::ostringstream stream;
            stream << file.rdbuf();
            if (file.bad()) {
                throw ShaderException("Failed while reading shader file: " + path);
            }
            return stream.str();
        };

        const std::string vertexCode = readSource(m_vert_shader_path);
        const std::string fragmentCode = readSource(m_frag_shader_path);
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        if (!vertexShader || !fragmentShader) {
            if (vertexShader) glDeleteShader(vertexShader);
            if (fragmentShader) glDeleteShader(fragmentShader);
            throw ShaderException("OpenGL failed to create shader objects");
        }

        try {
            compileShader(vertexShader, vertexCode, m_vert_shader_path);
            compileShader(fragmentShader, fragmentCode, m_frag_shader_path);
            linkProgram(vertexShader, fragmentShader);
        } catch (...) {
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            throw;
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void Shader::compileShader(GLuint shader, const std::string& code,
                               const std::string& sourcePath) {
        const char* source = code.c_str();
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success == GL_TRUE) return;

        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> log(static_cast<size_t>(std::max(logLength, 1)));
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        throw ShaderException("Failed to compile '" + sourcePath + "': " + log.data());
    }

    GLint Shader::getUniformLocation(const GLchar *name) const {
        GLint location = glGetUniformLocation(m_shaderID, name);
        // Some drivers optimize out unused uniforms; avoid noisy warnings.
        return location;
    }

    void Shader::linkProgram(GLuint vertex, GLuint fragment) {
        m_shaderID = glCreateProgram();
        if (!m_shaderID) {
            throw ShaderException("OpenGL failed to create a shader program (vertex: " +
                                  m_vert_shader_path + ", fragment: " +
                                  m_frag_shader_path + ")");
        }
        glAttachShader(m_shaderID, vertex);
        glAttachShader(m_shaderID, fragment);
        glLinkProgram(m_shaderID);
        GLint success;
        glGetProgramiv(m_shaderID, GL_LINK_STATUS, &success);
        if (!success) {
            GLint logLength = 0;
            glGetProgramiv(m_shaderID, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<GLchar> log(static_cast<size_t>(std::max(logLength, 1)));
            glGetProgramInfoLog(m_shaderID, logLength, nullptr, log.data());
            const std::string message = "Failed to link shader program (vertex: " + m_vert_shader_path +
                                        ", fragment: " + m_frag_shader_path + "): " + log.data();
            glDeleteProgram(m_shaderID);
            m_shaderID = 0;
            throw ShaderException(message);
        }
    }

} // namespace Graphics
