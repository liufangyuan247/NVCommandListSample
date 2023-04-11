#version 460 core

#extension GL_NV_command_list : enable

layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aColor;

layout (binding = 0, std140, commandBindableNV) uniform SceneData {
  mat4 VP;
};

layout (binding = 1, std140, commandBindableNV) uniform ObjectData {
  mat4 M;
};

out Intepolator {
  vec4 color;
} vs_out;


void main() {
  gl_Position = VP * (M * aPos);
  vs_out.color = aColor;
}