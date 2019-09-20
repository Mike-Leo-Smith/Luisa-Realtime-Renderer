//
// Created by mike on 19-5-9.
//

#ifndef LEARNOPENGL_SCENE_H
#define LEARNOPENGL_SCENE_H

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <core/shader.h>

#include "common.h"

namespace impl {

struct AABB {
    glm::vec3 min{1.e6f};
    glm::vec3 max{-1.e6f};
};

}

class SceneInfo {

public:
    using Camera = impl::CameraInfo;
    using Light = impl::LightInfo;
    using Material = impl::MaterialInfo;
    using Mesh = impl::MeshInfo;
    using Animation = impl::AnimationKeyframeInfo;

private:
    std::string _folder;
    std::vector<Camera> _cameras;
    std::vector<Light> _lights;
    std::unordered_map<std::string, Material> _materials;
    std::unordered_map<std::string, std::vector<Animation>> _animations;
    std::vector<Mesh> _meshes;
    bool _animated{false};
    
    SceneInfo() = default;

public:
    static SceneInfo load(const std::string &path);
    void print() const noexcept;
    [[nodiscard]] const std::string &folder() const noexcept { return _folder; }
    [[nodiscard]] const std::vector<Camera> &cameras() const noexcept { return _cameras; }
    [[nodiscard]] const std::vector<Light> &lights() const noexcept { return _lights; }
    [[nodiscard]] const std::unordered_map<std::string, Material> &materials() const noexcept { return _materials; }
    [[nodiscard]] const std::vector<Mesh> &meshes() const noexcept { return _meshes; }
    [[nodiscard]] const std::unordered_map<std::string, std::vector<Animation>> &animations() const noexcept { return _animations; }
    [[nodiscard]] bool animated() const noexcept { return _animated; }
    
};

class Geometry {

public:
    using AABB = impl::AABB;

private:
    std::vector<size_t> _mesh_offsets;
    std::vector<size_t> _mesh_sizes;
    std::vector<std::string> _mesh_animation_names;
    AABB _aabb{};
    size_t _triangle_count{0};
    size_t _vertex_count{0};
    size_t _texture_count{0};
    uint32_t _vertex_array{0};
    uint32_t _position_buffer{0};
    uint32_t _normal_buffer{0};
    uint32_t _color_buffer{0};
    uint32_t _tex_coord_buffer{0};
    uint32_t _gloss_buffer{0};
    uint32_t _tex_property_buffer{0};
    uint32_t _texture_array{0};
    
    Geometry() = default;
    
    template<typename T>
    static T _flatten(const T &v, const std::vector<glm::uvec3> &indices) noexcept {
        T container;
        container.reserve(indices.size() * 3);
        for (auto i : indices) {
            container.emplace_back(v[i.x]);
            container.emplace_back(v[i.y]);
            container.emplace_back(v[i.z]);
        }
        return container;
    }

public:
    static Geometry create(const SceneInfo &info);
    
    ~Geometry();
    Geometry(Geometry &&) = default;
    Geometry(const Geometry &) = delete;
    Geometry &operator=(Geometry &&) = default;
    Geometry &operator=(const Geometry &) = delete;
    
    [[nodiscard]] AABB aabb() const noexcept { return _aabb; }
    [[nodiscard]] const std::vector<size_t> &mesh_offsets() const noexcept { return _mesh_offsets; }
    [[nodiscard]] const std::vector<size_t> &mesh_sizes() const noexcept { return _mesh_sizes; }
    [[nodiscard]] const std::vector<std::string> &mesh_animation_names() const noexcept { return _mesh_animation_names; }
    [[nodiscard]] uint32_t position_buffer_id() const noexcept { return _position_buffer; }
    [[nodiscard]] uint32_t normal_buffer_id() const noexcept { return _normal_buffer; }
    [[nodiscard]] size_t texture_count() const noexcept { return _texture_count; }
    void render(const Shader &shader) const;
    void shadow(const Shader &shader) const;
    
};

#endif //LEARNOPENGL_SCENE_H
