#version 460 core

#include "common.h"

out vec4 fragColor;

void main() {
  fragColor = object.color;
}