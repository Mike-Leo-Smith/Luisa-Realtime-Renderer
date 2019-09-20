//
// Created by mike on 19-5-11.
//

#ifndef LEARNOPENGL_COMMON_H
#define LEARNOPENGL_COMMON_H

#include <string>
#include <glm/glm.hpp>

namespace impl {

struct CameraInfo {
    float time{0.0f};
    glm::vec3 eye{0.0f, 0.0f, 1.0f};
    glm::vec3 lookat{0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    
    CameraInfo() = default;
    
    CameraInfo(float time, glm::vec3 eye, glm::vec3 lookat, glm::vec3 up)
        : time{time}, eye{eye}, lookat{lookat}, up{up} {}
};

struct LightInfo {
    glm::vec3 position{0.0f};
    glm::vec3 emission{1.0f};
    glm::vec3 axis_direction{0.0f, 1.0f, 0.0f};
    glm::vec3 axis_position{0.0f, 0.0f, 0.0f};
    float angular_v{0.0f};
    float radius{0.001f};
};

struct MaterialInfo {
    glm::vec3 color{1.0f};
    float specular{0.0f};
    float roughness{128.0f};
    std::string file_name;
};

struct AnimationKeyframeInfo {
    float time{0.0f};
    glm::mat4 transform{1.0f};
    std::string file_name;
};

struct MeshInfo {
    glm::mat4 transform{1.0f};
    std::string file_name{};
    std::string material_name{};
    std::string animation_name{};
};

}

#endif //LEARNOPENGL_COMMON_H
