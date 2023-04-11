#version 460 core

in vec2 texcoord;
in float alpha;
out vec4 color;

layout (binding = 0) uniform sampler2D tex0;

void main() {
  const float float_epsilon = 0.00001;
  if  (alpha > -float_epsilon && alpha < 1.0 + float_epsilon) {
    color = vec4(vec3(texture(tex0, texcoord)), alpha);  
  } else {
    color = texture(tex0, texcoord);
  }
}