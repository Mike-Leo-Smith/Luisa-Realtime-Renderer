//
// Created by Mike Smith on 2019/9/18.
//

#ifndef LEARNOPENGL_TEXTURE_PACKER_H
#define LEARNOPENGL_TEXTURE_PACKER_H

#include <array>
#include <queue>
#include <random>
#include <iostream>
#include <unordered_map>
#include <type_traits>
#include <functional>
#include <string_view>
#include <glm/glm.hpp>
#include <stb_image.h>

#include "util.h"
#include "serialize.h"

class TexturePacker {

public:
    struct Quad {
        uint32_t index{};
        uint32_t x{};
        uint32_t y{};
        uint32_t size{};
        
        constexpr Quad() noexcept = default;
        constexpr Quad(uint32_t index, uint32_t x, uint32_t y, uint32_t size) noexcept : index{index}, x{x}, y{y}, size{size} {}
    };
    
    struct ImageBlock {
        uint32_t index;
        glm::uvec2 offset;
        glm::uvec2 size;
        constexpr ImageBlock() noexcept : index{}, offset{}, size{} {}
        constexpr ImageBlock(uint32_t index, glm::uvec2 offset, glm::uvec2 size) noexcept : index{index}, offset{offset}, size{size} {}
    };

private:
    size_t _max_size{};
    size_t _min_size{};
    size_t _max_level_count{};
    std::vector<std::queue<Quad>> _available_quads;
    std::vector<std::vector<glm::u8vec4>> _image_buffers;
    std::unordered_map<std::string, ImageBlock> _loaded_images;
    
    Quad _decompose_quad(Quad quad, size_t target_size, size_t level) noexcept;
    Quad _fit_image(size_t w, size_t h) noexcept;
    void _fill(TexturePacker::ImageBlock b, const glm::u8vec4 *data) noexcept;

public:
    explicit TexturePacker(size_t max_size = 4096ul, size_t min_size = 16ul);
    ImageBlock load(const std::string &path);
    [[nodiscard]] size_t count() const noexcept;
    [[nodiscard]] size_t max_size() const noexcept;
    [[nodiscard]] const std::vector<glm::u8vec4> &image_buffer(size_t index) const noexcept;
    [[nodiscard]] uint32_t create_opengl_texture_array() const noexcept;
    
};

#endif //LEARNOPENGL_TEXTURE_PACKER_H
