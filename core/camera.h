#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
 public:
  Camera() = default;

  void set_target(const glm::vec3& target) { target_ = target; }
  const glm::vec3& target() const { return target_; }

  void set_look_pitch_yaw(const glm::vec2& look_pitch_yaw) {
    look_pitch_yaw_ = look_pitch_yaw;
  }
  const glm::vec2& look_pitch_yaw() const { return look_pitch_yaw_; }

  void set_distance(float distance) { distance_ = distance; }
  float distance() const { return distance_; }

  glm::vec3 forward() const {
    glm::vec2 look_pitch_yaw_rad = glm::radians(look_pitch_yaw_);
    return {
        glm::cos(look_pitch_yaw_rad.x) * glm::sin(look_pitch_yaw_rad.y),
        glm::cos(look_pitch_yaw_rad.x) * glm::cos(look_pitch_yaw_rad.y),
        glm::sin(look_pitch_yaw_rad.x),
    };
  }

  glm::vec3 right() const {
    glm::vec3 up = glm::cross(forward(), glm::vec3(0, 0, 1));
    return glm::normalize(up);
  }

  glm::mat4 view() const {
    return glm::lookAt(target_ - distance_ * forward(), target_,
                       glm::vec3(0, 0, 1));
  }

 private:
  glm::vec3 target_{0, 0, 0};
  glm::vec2 look_pitch_yaw_{0, 0};
  float distance_ = 10.0f;
};
