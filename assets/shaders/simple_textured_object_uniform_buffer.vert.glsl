#version 460 core

#include "common.h"

layout(location=0) in vec4 in_position;
layout(location=2) in vec2 in_texcoord;

out vec2 texcoord;

void main() {
  gl_Position = scene.VP * (object.M * in_position);
  texcoord = in_texcoord;
}