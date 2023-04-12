#ifndef COMMON_H
#define COMMON_H

#define POSITION 0
#define COLOR 1
#define UV 2

#define UBO_SCENE 0
#define UBO_OBJECT 1

#if defined(GL_core_profile) || defined(GL_compatibility_profile) || defined(GL_es_profile)

#extension GL_ARB_bindless_texture : require
#extension GL_NV_command_list : enable
#if GL_NV_command_list
layout(commandBindableNV) uniform;
#endif

#endif

#ifdef __cplusplus
namespace common
{
  using namespace glm;
#endif

struct SceneData {
  mat4 VP;
};

struct ObjectData {
  mat4 M;
  vec4 color;
};

#ifdef __cplusplus
} // namespace
#endif

#if defined(GL_core_profile) || defined(GL_compatibility_profile) || defined(GL_es_profile)

layout(std140,binding=UBO_SCENE) uniform sceneBuffer {
  SceneData   scene;
};

layout(std140,binding=UBO_OBJECT) uniform objectBuffer {
  ObjectData  object;
};

#endif


#endif