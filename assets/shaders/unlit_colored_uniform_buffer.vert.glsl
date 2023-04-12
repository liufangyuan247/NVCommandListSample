#version 460 core

#include "common.h"

layout (location = 0) in vec4 aPos;

void main() {
  gl_Position = scene.VP * (object.M * aPos);
}