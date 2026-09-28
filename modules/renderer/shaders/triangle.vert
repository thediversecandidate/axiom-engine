#version 450
// Stage 1 triangle: world-space position and linear vertex color; MVP from a push constant.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(push_constant) uniform Push {
  mat4 mvp; // column-major, column vectors: clip = mvp * vec4(p, 1) (CONVENTIONS §3-4)
} pc;

layout(location = 0) out vec3 vColor;

void main() {
  gl_Position = pc.mvp * vec4(inPosition, 1.0);
  vColor = inColor;
}
