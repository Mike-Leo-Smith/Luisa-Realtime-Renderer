//
// Created by Mike Smith on 2019/9/18.
//

#ifndef LEARNOPENGL_UTIL_H
#define LEARNOPENGL_UTIL_H

#include <cstdint>

namespace util {

namespace impl {

constexpr size_t log2_exact(size_t x) noexcept {
    auto index = 0ull;
    for (; x != 1ull; x >>= 1ull) { index++; }
    return index;
}

}

constexpr size_t next_power_of_two(size_t x) noexcept {
    if (x == 0) { return 0; }
    x--;
    x |= x >> 1u;
    x |= x >> 2u;
    x |= x >> 4u;
    x |= x >> 8u;
    x |= x >> 16u;
    x |= x >> 32u;
    return ++x;
}

constexpr size_t log2(size_t x) noexcept {
    return impl::log2_exact(next_power_of_two(x));
}

template<typename T>
inline T lerp(T u, T v, float t) {
    return (1 - t) * u + t * v;
}

inline glm::vec3 slerp(glm::vec3 n1, glm::vec3 n2, float t) {
    
    using namespace glm;
    
    auto l1 = length(n1);
    auto l2 = length(n2);
    auto length = lerp(l1, l2, t);
    
    n1 = normalize(n1);
    n2 = normalize(n2);
    
    if (dot(n1, n2) > 0.9995f) {
        return lerp(n1, n2, t) * length;
    }
    
    float theta = std::acos(dot(n1, n2));
    float sin_t_theta = std::sin(t * theta);
    return normalize(lerp(n1, n2, sin_t_theta / (sin_t_theta + std::sin((1 - t) * theta)))) * length;
    
}

template<typename OS>
OS &operator<<(OS &os, glm::vec3 v) {
    return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}

template<typename T>
class MemoryGuard {
private:
    T *_data;
    std::function<void(T *)> _deleter;
    
public:
    template<typename F>
    constexpr MemoryGuard(T *data, F &&deleter) noexcept : _data{data}, _deleter{std::forward<F>(deleter)} {}
    
    ~MemoryGuard() noexcept { _deleter(_data); }
};

template<typename T, typename F>
constexpr MemoryGuard<std::decay_t<T>> make_memory_guard(T *data, F &&deleter) noexcept {
    return {data, std::forward<F>(deleter)};
}

}

#endif //LEARNOPENGL_UTIL_H
