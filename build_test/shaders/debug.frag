#version 460 core

in  vec2 vUV;
out vec4 fragColor;

uniform float uTime;

void main() {
    // Simple gradient - red to green, animated blue
    fragColor = vec4(vUV.x, vUV.y, 0.5 + 0.5 * sin(uTime * 2.0), 1.0);
}
