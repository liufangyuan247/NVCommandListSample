#version 460 core

layout(location=0) in vec4 in_position;
layout(location=2) in vec2 in_texcoord;

out vec2 texcoord;
out float alpha;
uniform mat4 VP;
uniform mat4 M;
uniform float in_alpha;

void main() {
  gl_Position = VP * (M * in_position);
  texcoord = in_texcoord;
  alpha = in_alpha;
}