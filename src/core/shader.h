#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <regex>
#include <string_view>

#include <glsl/glsl_optimizer.h>
#include <core/serialize.h>

class Shader {
public:
    
    using TemplateList = std::map<std::string, std::string>;
    
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const std::string &vertexPath, const std::string &fragmentPath, const std::string &geometryPath = {}, const TemplateList &tl = {}) {
        
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        
        vertexCode = optimizeShaderSource(readSourceFile(vertexPath, tl), kGlslOptShaderVertex);
        fragmentCode = optimizeShaderSource(readSourceFile(fragmentPath, tl), kGlslOptShaderFragment);
        
        // if geometry shader path is present, also load a geometry shader
        if (!geometryPath.empty()) {
            geometryCode = readSourceFile(geometryPath, tl);
        }
        
        const char *vShaderCode = vertexCode.c_str();
        const char *fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // if geometry shader is given, compile geometry shader
        unsigned int geometry;
        if (!geometryPath.empty()) {
            const char *gShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (!geometryPath.empty())
            glAttachShader(ID, geometry);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (!geometryPath.empty())
            glDeleteShader(geometry);
        
    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use() {
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string &name, const glm::vec2 &value) const {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string &name, float x, float y) const {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string &name, const glm::vec3 &value) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string &name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const glm::vec4 &value) const {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string &name, float x, float y, float z, float w) {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string &name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string &name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    
    static std::string replaceVersionString(std::string &src, std::string_view replacement) {
        static std::regex version_string_finder{R"(#version\s+\d{3}\s+(core|es)?)", std::regex::optimize | std::regex::ECMAScript};
        std::smatch match_result{};
        if (!std::regex_search(src, match_result, version_string_finder)) {
            throw std::runtime_error{"Failed to find version string in shader source"};
        }
        auto version_string = match_result.str();
        src = serialize(
            std::string_view{src}.substr(0, match_result.position()),
            replacement,
            std::string_view{src}.substr(match_result.position() + match_result.length()));
        return version_string;
    }
    
    static std::string optimizeShaderSource(std::string src, glslopt_shader_type shader_type) {
        
        auto version_string = replaceVersionString(src, "#version 300 es");
        
        static constexpr auto context_deleter = [](glslopt_ctx *ctx) noexcept { glslopt_cleanup(ctx); };
        static std::unique_ptr<glslopt_ctx, decltype(context_deleter)> optimizer_context{glslopt_initialize(kGlslTargetOpenGL), context_deleter};
        
        static constexpr auto shader_deleter = [](glslopt_shader *shader) noexcept { glslopt_shader_delete(shader); };
        std::unique_ptr<glslopt_shader, decltype(shader_deleter)> shader{glslopt_optimize(optimizer_context.get(), shader_type, src.c_str(), 0), shader_deleter};
        
        if (!glslopt_get_status(shader.get())) {
            throw std::runtime_error{serialize("Failed to optimize shader: ", glslopt_get_log(shader.get()))};
        }
        src = glslopt_get_output(shader.get());
        replaceVersionString(src, version_string);
        return src;
    }
    
    static std::string readSourceFile(const std::string &path, const TemplateList &tl) {
        
        std::ifstream file{path};
        
        if (!file.is_open()) {
            std::cerr << "Failed to read file: " << path << std::endl;
            return {};
        }
        
        std::string source;
        std::string templ;
        std::string line;
        while (!file.eof()) {
            auto c = file.get();
            if (c == '$') {  // begin of template
                if (file.get() != '{') {
                    std::cerr << "Expected '{' in shader template: " << path << std::endl;
                    return {};
                }
                templ.clear();
                while (!file.eof()) {
                    templ.push_back(file.get());
                    if (templ.back() == '}') {
                        break;
                    }
                }
                if (templ.back() != '}') {
                    std::cerr << "Expected '}' in shader template: " << path << std::endl;
                    return {};
                }
                templ.pop_back();
                auto iter = tl.find(templ);
                if (iter == tl.end()) {
                    std::cerr << "Unknown template name '" << templ << "': " << path << std::endl;
                    return {};
                }
                source.append(iter->second);
            } else if (c != EOF) {
                source.push_back(c);
            }
        }
        
        return source;
    }
    
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    static void checkCompileErrors(GLuint shader, const std::string &type) {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
#endif