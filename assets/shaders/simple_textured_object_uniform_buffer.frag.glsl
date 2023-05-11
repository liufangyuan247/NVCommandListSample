#version 460 core

#include "common.h"

in vec2 texcoord;
out vec4 color;

void main() {
  float alpha = object.color.a;
  const float float_epsilon = 0.00001;
  if  (alpha > -float_epsilon && alpha < 1.0 + float_epsilon) {
    color = vec4(vec3(texture(material.texture, texcoord)), alpha);  
  } else {
    color = texture(material.texture, texcoord);
  }
}