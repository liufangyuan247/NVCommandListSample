#version 460 core

layout (location = 0) in vec4 aPos;
layout (location = 1) in vec4 aColor;

layout (binding = 0, std140) uniform SceneData {
  mat4 VP;
};

layout (binding = 1, std140) uniform ObjectData {
  mat4 M;
};

out Intepolator {
  vec4 color;
} vs_out;


void main() {
  gl_Position = VP * (M * aPos);
  vs_out.color = aColor;
}