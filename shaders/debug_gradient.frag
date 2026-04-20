#version 460 core

in  vec2 vUV;
out vec4 fragColor;

uniform float uTime;

void main() {
    // Simple gradient to verify quad is rendering
    vec3 col = vec3(vUV.x, vUV.y, 0.5 + 0.5 * sin(uTime));
    fragColor = vec4(col, 1.0);
}
