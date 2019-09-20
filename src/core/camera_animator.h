//
// Created by mike on 19-5-12.
//

#ifndef LEARNOPENGL_CAMERA_ANIMATOR_H
#define LEARNOPENGL_CAMERA_ANIMATOR_H

#include <vector>
#include "common.h"
#include "scene.h"

class CameraAnimator {

public:
    using State = impl::CameraInfo;

private:
    std::vector<State> _states;
    explicit CameraAnimator(std::vector<State> states) : _states{std::move(states)} {}

public:
    static CameraAnimator create(const SceneInfo &info);
    bool empty() const noexcept { return _states.empty(); }
    State state(float time) const noexcept;

};

#endif //LEARNOPENGL_CAMERA_ANIMATOR_H
