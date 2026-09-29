#version 450
// Writes the interpolated linear color; the R8G8B8A8_SRGB target applies the sRGB encoding.
layout(location = 0) in vec3 vColor;
layout(location = 0) out vec4 outColor;

void main() {
  outColor = vec4(vColor, 1.0);
}
