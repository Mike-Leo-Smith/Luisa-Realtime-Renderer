//
// Created by mike on 19-5-9.
//

#include <functional>
#include <exception>
#include <algorithm>
#include <queue>
#include <iostream>
#include <memory>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "serialize.h"
#include "util.h"
#include "scene.h"
#include "texture_packer.h"

SceneInfo SceneInfo::load(const std::string &path) {
    
    SceneInfo scene;
    scene._folder = path.substr(0, path.find_last_of('/')).append("/");
    
    std::ifstream file{path};
    
    auto read_token = [&] {
        
        auto do_read_token = [&] {
            if (file.eof()) {
                throw std::runtime_error{"Unexpected EOF"};
            }
            std::string token;
            file >> token;
            return token;
        };
        
        auto token = do_read_token();
        while (token.front() == '#') {
            std::getline(file, token);
            token = do_read_token();
        }
        
        return token;
    };
    
    auto match_token = [&](const std::string &expected) {
        auto token = read_token();
        if (token != expected) {
            throw std::runtime_error{serialize("Bad token: expected \"", expected, "\", got \"", token, "\"\n")};
        }
        return token;
    };
    
    auto read_number = [&] {
        auto number = 0.0f;
        std::istringstream{read_token()} >> number;
        return number;
    };
    
    auto read_vec3 = [&] {
        auto x = read_number();
        auto y = read_number();
        auto z = read_number();
        return glm::vec3(x, y, z);
    };
    
    auto read_mat4 = [&] {
        glm::mat4 m;
        auto p = glm::value_ptr(m);
        for (auto i = 0; i < 16; i++) {
            p[i] = read_number();
        }
        return m;
    };
    
    std::unordered_map<std::string, std::function<void()>> parsers;
    
    parsers.emplace("camera", [&] {
        match_token("{");
        Camera camera;
        while (true) {
            auto token = read_token();
            if (token == "time") {
                camera.time = read_number();
            } else if (token == "eye") {
                camera.eye = read_vec3();
            } else if (token == "lookat") {
                camera.lookat = read_vec3();
            } else if (token == "up") {
                camera.up = read_vec3();
            } else if (token == "}") {
                break;
            } else {
                throw std::runtime_error{serialize("Unknown token: ", token)};
            }
        }
        auto dir = camera.lookat - camera.eye;
        camera.up = glm::normalize(glm::cross(glm::cross(dir, camera.up), dir));
        scene._cameras.emplace_back(camera);
    });
    
    parsers.emplace("light", [&] {
        match_token("{");
        Light light;
        while (true) {
            auto token = read_token();
            if (token == "emission") {
                light.emission = read_vec3();
            } else if (token == "position") {
                light.position = read_vec3();
            } else if (token == "radius") {
                light.radius = read_number();
            } else if (token == "axis_direction") {
                light.axis_direction = read_vec3();
            } else if (token == "axis_position") {
                light.axis_position = read_vec3();
            } else if (token == "angular_v") {
                light.angular_v = glm::radians(read_number());
            } else if (token == "}") {
                break;
            } else {
                throw std::runtime_error{serialize("Unknown token: ", token)};
            }
        }
        scene._lights.emplace_back(light);
    });
    
    parsers.emplace("material", [&] {
        std::string material_name;
        try {
            match_token("{");
        } catch (const std::runtime_error &e) {
            material_name = read_token();
            match_token("{");
        }
        Material material;
        while (true) {
            auto token = read_token();
            if (token == "color") {
                material.color = read_vec3();
            } else if (token == "file" || token == "albedoTex") {
                material.file_name = read_token();
            } else if (token == "name") {
                material_name = read_token();
            } else if (token == "specular") {
                material.specular = read_number();
            } else if (token == "roughness") {
                material.roughness = read_number();
            } else if (token == "}") {
                break;
            } else {
                throw std::runtime_error{serialize("Unknown token: ", token)};
            }
        }
        scene._materials.emplace(std::move(material_name), std::move(material));
    });
    
    parsers.emplace("mesh", [&] {
        match_token("{");
        Mesh mesh;
        while (true) {
            auto token = read_token();
            if (token == "file") {
                mesh.file_name = read_token();
            } else if (token == "transform") {
                mesh.transform = read_mat4();
            } else if (token == "translate") {
                mesh.transform = glm::translate(glm::mat4{1.0f}, read_vec3()) * mesh.transform;
            } else if (token == "rotate") {
                auto r = read_vec3();
                mesh.transform = glm::rotate(glm::mat4{1.0f}, glm::radians(r.y), glm::vec3{0.0f, 1.0f, 0.0f}) * mesh.transform;
                mesh.transform = glm::rotate(glm::mat4{1.0f}, glm::radians(r.x), glm::vec3{1.0f, 0.0f, 0.0f}) * mesh.transform;
                mesh.transform = glm::rotate(glm::mat4{1.0f}, glm::radians(r.z), glm::vec3{0.0f, 0.0f, 1.0f}) * mesh.transform;
            } else if (token == "scale") {
                mesh.transform = glm::scale(glm::mat4{1.0f}, read_vec3()) * mesh.transform;
            } else if (token == "file") {
                mesh.file_name = read_token();
            } else if (token == "material") {
                mesh.material_name = read_token();
            } else if (token == "animation") {
                mesh.animation_name = read_token();
            } else if (token == "}") {
                break;
            } else {
                throw std::runtime_error{serialize("Unknown token: ", token)};
            }
        }
        scene._meshes.emplace_back(std::move(mesh));
    });
    
    parsers.emplace("animation", [&] {
        match_token("{");
        Animation animation;
        std::string name;
        while (true) {
            auto token = read_token();
            if (token == "name") {
                name = read_token();
            } else if (token == "time") {
                animation.time = read_number();
            } else if (token == "transform") {
                animation.transform = read_mat4();
            } else if (token == "translate") {
                animation.transform = glm::translate(glm::mat4{1.0f}, read_vec3()) * animation.transform;
            } else if (token == "rotate") {
                auto r = read_vec3();
                animation.transform = glm::rotate(glm::mat4{1.0f}, glm::radians(r.y), glm::vec3{0.0f, 1.0f, 0.0f}) * animation.transform;
                animation.transform = glm::rotate(glm::mat4{1.0f}, glm::radians(r.x), glm::vec3{1.0f, 0.0f, 0.0f}) * animation.transform;
                animation.transform = glm::rotate(glm::mat4{1.0f}, glm::radians(r.z), glm::vec3{0.0f, 0.0f, 1.0f}) * animation.transform;
            } else if (token == "scale") {
                animation.transform = glm::scale(glm::mat4{1.0f}, read_vec3()) * animation.transform;
            } else if (token == "file") {
                animation.file_name = read_token();
            } else if (token == "}") {
                break;
            } else {
                throw std::runtime_error{serialize("Unknown token: ", token)};
            }
        }
        auto iter = scene._animations.find(name);
        if (iter == scene._animations.end()) {
            iter = scene._animations.emplace(name, std::vector<Animation>{}).first;
        }
        iter->second.emplace_back(std::move(animation));
    });
    
    while (!file.eof()) {
        auto token = read_token();
        if (!token.empty()) {
            auto iter = parsers.find(token);
            if (iter == parsers.end()) {
                throw std::runtime_error{serialize("Unsupported component: ", token)};
            }
            iter->second();
        }
    }
    
    std::sort(scene._cameras.begin(), scene._cameras.end(), [](const Camera &lhs, const Camera &rhs) {
        return lhs.time < rhs.time;
    });
    
    for (auto &&item : scene._animations) {
        auto &&nodes = item.second;
        std::sort(nodes.begin(), nodes.end(), [](const Animation &lhs, const Animation &rhs) {
            return lhs.time < rhs.time;
        });
    }
    
    scene._animated = !scene._animations.empty() || std::any_of(scene._lights.cbegin(), scene._lights.cend(), [](const Light &light) {
        return light.angular_v != 0.0f;
    });
    
    return scene;
}

void SceneInfo::print() const noexcept {
    
    auto vec2str = [](glm::vec3 v) {
        return serialize("(", v.x, ", ", v.y, ", ", v.z, ")");
    };
    
    auto mat2str = [](glm::mat4 m) {
        std::string s{"("};
        for (auto row = 0; row < 4; row++) {
            s.append("(");
            for (auto col = 0; col < 4; col++) {
                s.append(std::to_string(m[row][col])).append(", ");
            }
            s.pop_back();
            s.pop_back();
            s.append("), ");
        }
        s.pop_back();
        s.pop_back();
        return s.append(")");
    };
    
    std::cout << "Folder [ " << _folder << " ]\n\n";
    
    std::cout << "Cameras [\n";
    for (auto &&camera : _cameras) {
        std::cout << "  { "
                  << "time = " << camera.time << ", "
                  << "eye = " << vec2str(camera.eye) << ", "
                  << "lookat = " << vec2str(camera.eye) << ", "
                  << "up = " << vec2str(camera.up) << " }\n";
    }
    std::cout << "]\n\n";
    
    std::cout << "Lights [\n";
    for (auto &&light : _lights) {
        std::cout << "  { "
                  << "position = " << vec2str(light.position) << ", "
                  << "emission = " << vec2str(light.emission) << ", "
                  << "radius = " << light.radius << " }\n";
    }
    std::cout << "]\n\n";
    
    std::cout << "Materials [\n";
    for (auto &&item : _materials) {
        auto &&name = item.first;
        auto &&material = item.second;
        std::cout << "  { "
                  << "name = " << name << ", "
                  << "color = " << vec2str(material.color) << ", "
                  << "specular = " << material.specular << ", "
                  << "roughness = " << material.roughness << ", "
                  << "file = " << (material.file_name.empty() ? "<None>" : material.file_name) << " }\n";
    }
    std::cout << "]\n\n";
    
    std::cout << "Meshes [\n";
    for (auto &&mesh : _meshes) {
        std::cout << "  { "
                  << "file = " << mesh.file_name << ", "
                  << "material = " << (mesh.material_name.empty() ? "<Emeded>" : mesh.material_name) << ", "
                  << "transform = " << mat2str(mesh.transform) << ", "
                  << "animation = " << (mesh.animation_name.empty() ? "<None>" : mesh.animation_name) << " }\n";
    }
    std::cout << "]\n\n";
    
    std::cout << "Animations [\n";
    for (auto &&item : _animations) {
        auto &&name = item.first;
        auto &&nodes = item.second;
        for (auto &&node : nodes) {
            std::cout << "  { "
                      << "name = " << name << ", "
                      << "time = " << node.time << ", "
                      << "file = " << (node.file_name.empty() ? "<None>" : node.file_name) << ", "
                      << "transform = " << mat2str(node.transform) << " }\n";
        }
    }
    std::cout << "]\n\n";
    
}

Geometry Geometry::create(const SceneInfo &info) {
    
    Geometry geometry;
    
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec3> tex_coords;
    std::vector<glm::vec4> tex_properties;
    std::vector<glm::vec2> glosses;  // (specular, roughness)
    std::vector<glm::uvec3> indices;
    
    TexturePacker packer;
    
    for (auto &&mesh : info.meshes()) {
        
        auto path = info.folder() + mesh.file_name;
        
        Assimp::Importer importer;
        auto ai_scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_FixInfacingNormals | aiProcess_GenSmoothNormals);
        if (ai_scene == nullptr || (ai_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || ai_scene->mRootNode == nullptr) {
            throw std::runtime_error{serialize("Failed to load scene from: ", path)};
        }
        
        geometry._mesh_offsets.emplace_back(positions.size());
        geometry._mesh_animation_names.emplace_back(mesh.animation_name);
        
        auto offset = static_cast<uint32_t>(geometry._mesh_offsets.back());
        
        // gather submeshes
        std::vector<aiMesh *> mesh_list;
        std::queue<aiNode *> node_queue;
        node_queue.push(ai_scene->mRootNode);
        while (!node_queue.empty()) {
            auto node = node_queue.front();
            node_queue.pop();
            for (auto i = 0ul; i < node->mNumMeshes; i++) {
                mesh_list.emplace_back(ai_scene->mMeshes[node->mMeshes[i]]);
            }
            for (auto i = 0ul; i < node->mNumChildren; i++) {
                node_queue.push(node->mChildren[i]);
            }
        }
        
        // process submeshes
        for (auto ai_mesh : mesh_list) {
            
            // process material
            std::string tex_name;
            glm::vec3 color{1.0f};
            glm::vec2 gloss{0.0f, 0.0f};
            if (!mesh.material_name.empty()) {
                auto iter = info.materials().find(mesh.material_name);
                if (iter == info.materials().end()) {
                    throw std::runtime_error{serialize("Reference to undefined material: ", mesh.material_name)};
                }
                auto &&material = iter->second;
                if (material.file_name.empty()) {
                    color = material.color;
                } else {
                    tex_name = material.file_name;
                }
                gloss.x = material.specular;
                gloss.y = material.roughness;
            } else {
                auto ai_material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
                if (ai_material->GetTextureCount(aiTextureType_DIFFUSE) == 0) {
                    aiColor3D ai_color;
                    ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, ai_color);
                    color = glm::vec3{ai_color.r, ai_color.g, ai_color.b};
                } else {
                    aiString ai_path;
                    ai_material->GetTexture(aiTextureType_DIFFUSE, 0, &ai_path);
                    tex_name = mesh.file_name;
                    tex_name.erase(tex_name.find('/') + 1).append(ai_path.C_Str());
                }
                float shininess;
                aiColor3D specular;
                ai_material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
                ai_material->Get(AI_MATKEY_SHININESS, shininess);
                gloss.x = (specular.r + specular.g + specular.b) / 3.0f;
                gloss.y = std::sqrt(2.0f / (2.0f + shininess));
            }
            
            auto has_texture = false;
            TexturePacker::ImageBlock block{};
            if (!tex_name.empty()) {
                block = packer.load(info.folder() + tex_name);
                has_texture = true;
            }
            
            auto model_matrix = mesh.transform;
            auto normal_matrix = glm::transpose(glm::inverse(glm::mat3{model_matrix}));
            
            // process vertices
            for (auto i = 0ul; i < ai_mesh->mNumVertices; i++) {
                auto ai_position = ai_mesh->mVertices[i];
                auto ai_normal = ai_mesh->mNormals[i];
                auto position = glm::vec3{model_matrix * glm::vec4{ai_position.x, ai_position.y, ai_position.z, 1.0f}};
                auto normal = normal_matrix * glm::vec3{ai_normal.x, ai_normal.y, ai_normal.z};
                positions.emplace_back(position);
                normals.emplace_back(normal);
                auto ai_tex_coords = ai_mesh->mTextureCoords[0];
                if (!has_texture || ai_tex_coords == nullptr) {
                    tex_coords.emplace_back(0.0f, 0.0f, -1.0f);
                    tex_properties.emplace_back(0.0f, 0.0f, 0.0f, 0.0f);
                } else {
                    tex_coords.emplace_back(ai_tex_coords[i].x, ai_tex_coords[i].y, block.index);
                    tex_properties.emplace_back(block.offset.x, block.offset.y, block.size.x, block.size.y);
                }
                colors.emplace_back(color);
                glosses.emplace_back(gloss);
                geometry._aabb.min = glm::min(geometry._aabb.min, position);
                geometry._aabb.max = glm::max(geometry._aabb.max, position);
            }
            
            // process faces
            for (auto i = 0ul; i < ai_mesh->mNumFaces; i++) {
                auto &&face = ai_mesh->mFaces[i].mIndices;
                indices.emplace_back(glm::uvec3{face[0], face[1], face[2]} + offset);
            }
            
            offset += ai_mesh->mNumVertices;
        }
        geometry._mesh_sizes.emplace_back(positions.size() - geometry._mesh_offsets.back());
    }
    
    geometry._texture_count = packer.count();
    geometry._texture_array = packer.create_opengl_texture_array();
    
    auto aabb_min = glm::min(geometry._aabb.min, geometry._aabb.max);
    auto aabb_max = glm::max(geometry._aabb.min, geometry._aabb.max);
    geometry._aabb.min = aabb_min;
    geometry._aabb.max = aabb_max;
    
    std::cout << "AABB: "
              << "min = (" << aabb_min.x << ", " << aabb_min.y << ", " << aabb_min.z << "), "
              << "max = (" << aabb_max.x << ", " << aabb_max.y << ", " << aabb_max.z << ")" << std::endl;
    
    geometry._triangle_count = indices.size();
    geometry._vertex_count = positions.size();
    
    std::cout << "Total vertices: " << positions.size() << std::endl;
    std::cout << "Total triangles: " << geometry._triangle_count << std::endl;
    
    // transfer to OpenGL
    glGenVertexArrays(1, &geometry._vertex_array);
    
    uint32_t buffers[6];
    glGenBuffers(6, buffers);
    
    geometry._position_buffer = buffers[0];
    geometry._normal_buffer = buffers[1];
    geometry._color_buffer = buffers[2];
    geometry._tex_coord_buffer = buffers[3];
    geometry._tex_property_buffer = buffers[4];
    geometry._gloss_buffer = buffers[5];
    
    glBindVertexArray(geometry._vertex_array);
    
    glBindBuffer(GL_ARRAY_BUFFER, geometry._position_buffer);
    glBufferData(GL_ARRAY_BUFFER, indices.size() * 3ul * sizeof(glm::vec3), _flatten(positions, indices).data(), GL_DYNAMIC_COPY);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    
    glBindBuffer(GL_ARRAY_BUFFER, geometry._normal_buffer);
    glBufferData(GL_ARRAY_BUFFER, indices.size() * 3ul * sizeof(glm::vec3), _flatten(normals, indices).data(), GL_DYNAMIC_COPY);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    
    glBindBuffer(GL_ARRAY_BUFFER, geometry._color_buffer);
    glBufferData(GL_ARRAY_BUFFER, indices.size() * 3ul * sizeof(glm::vec3), _flatten(colors, indices).data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    
    glBindBuffer(GL_ARRAY_BUFFER, geometry._tex_coord_buffer);
    glBufferData(GL_ARRAY_BUFFER, indices.size() * 3ul * sizeof(glm::vec3), _flatten(tex_coords, indices).data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    
    glBindBuffer(GL_ARRAY_BUFFER, geometry._tex_property_buffer);
    glBufferData(GL_ARRAY_BUFFER, indices.size() * 3ul * sizeof(glm::vec4), _flatten(tex_properties, indices).data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
    
    glBindBuffer(GL_ARRAY_BUFFER, geometry._gloss_buffer);
    glBufferData(GL_ARRAY_BUFFER, indices.size() * 3ul * sizeof(glm::vec2), _flatten(glosses, indices).data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
    
    glBindVertexArray(0);
    
    return geometry;
}

Geometry::~Geometry() {
    glDeleteVertexArrays(1, &_vertex_array);
    glDeleteBuffers(6, &_position_buffer);
    glDeleteTextures(1, &_texture_array);
}

void Geometry::render(const Shader &shader) const {
    glBindVertexArray(_vertex_array);
    glActiveTexture(GL_TEXTURE0);
    shader.setInt("textures", 0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _texture_array);
    glDrawArrays(GL_TRIANGLES, 0, _triangle_count * 3);
    glBindVertexArray(0);
}

void Geometry::shadow(const Shader &shader) const {
    glBindVertexArray(_vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, _triangle_count * 3);
    glBindVertexArray(0);
}
