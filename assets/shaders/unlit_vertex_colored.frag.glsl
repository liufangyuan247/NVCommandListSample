#version 460 core

out vec4 fragColor;

in Intepolator {
  vec4 color;
} fs_in;


void main() {
  fragColor = fs_in.color;
}