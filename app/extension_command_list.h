#pragma once

#include <cstdlib>

#include <GL/glew.h>

#pragma pack(push, 1)

typedef struct {
  uint header;
} TerminateSequenceCommandNV;

typedef struct {
  uint header;
} NOPCommandNV;

typedef struct {
  uint header;
  uint count;
  uint firstIndex;
  uint baseVertex;
} DrawElementsCommandNV;

typedef struct {
  uint header;
  uint count;
  uint first;
} DrawArraysCommandNV;

typedef struct {
  uint header;
  uint mode;
  uint count;
  uint instanceCount;
  uint firstIndex;
  uint baseVertex;
  uint baseInstance;
} DrawElementsInstancedCommandNV;

typedef struct {
  uint header;
  uint mode;
  uint count;
  uint instanceCount;
  uint first;
  uint baseInstance;
} DrawArraysInstancedCommandNV;

typedef struct {
  uint header;
  uint64_t address;
  uint typeSizeInByte;
} ElementAddressCommandNV;

typedef struct {
  uint header;
  uint index;
  uint64_t address;
} AttributeAddressCommandNV;

typedef struct {
  uint header;
  ushort index;
  ushort stage;
  uint64_t address;
} UniformAddressCommandNV;

typedef struct {
  uint header;
  float red;
  float green;
  float blue;
  float alpha;
} BlendColorCommandNV;

typedef struct {
  uint header;
  uint frontStencilRef;
  uint backStencilRef;
} StencilRefCommandNV;

typedef struct {
  uint header;
  float lineWidth;
} LineWidthCommandNV;

typedef struct {
  uint header;
  float scale;
  float bias;
} PolygonOffsetCommandNV;

typedef struct {
  uint header;
  float alphaRef;
} AlphaRefCommandNV;

typedef struct {
  uint header;
  uint x;
  uint y;
  uint width;
  uint height;
} ViewportCommandNV;  // only ViewportIndex 0

typedef struct {
  uint header;
  uint x;
  uint y;
  uint width;
  uint height;
} ScissorCommandNV;  // only ViewportIndex 0

typedef struct {
  uint header;
  uint frontFace;  // 0 for CW, 1 for CCW
} FrontFaceCommandNV;

#pragma pack(pop)
