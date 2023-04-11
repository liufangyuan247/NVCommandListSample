#version 460 core

layout (location = 0) in vec4 aPos;

uniform mat4 VP;
uniform mat4 M;

void main() {
  gl_Position = VP * (M * aPos);
}