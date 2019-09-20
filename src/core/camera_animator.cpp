//
// Created by mike on 19-5-12.
//

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "camera_animator.h"
#include "util.h"

CameraAnimator::State CameraAnimator::state(float time) const noexcept {
    
    auto start_time = _states.front().time;
    auto end_time = _states.back().time;
    auto total_time = end_time - start_time;
    auto delta_time = time - start_time;
    
    if (delta_time > total_time) {
        auto count = static_cast<int>(delta_time / total_time);
        delta_time = std::max(std::min(delta_time - count * total_time, end_time), start_time);
    }
    
    auto period_time = delta_time + start_time;
    auto iter = std::lower_bound(_states.cbegin(), _states.cend(), period_time, [](const State &state, float time) {
        return state.time < time;
    });
    auto &&after = *iter;
    auto &&before = iter == _states.cbegin() ? after : *(iter - 1);
    
    auto t = static_cast<float>((period_time - before.time) / (after.time - before.time));
    auto eye = util::lerp(before.eye, after.eye, t);
    auto lookat = eye + util::slerp(before.lookat - before.eye, after.lookat - after.eye, t);
    auto up = glm::normalize(util::slerp(before.up, after.up, t));
    
    return {time, eye, lookat, up};
}

CameraAnimator CameraAnimator::create(const SceneInfo &info) {
    return CameraAnimator(info.cameras());
}
