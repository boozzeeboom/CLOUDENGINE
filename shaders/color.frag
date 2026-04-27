#version 330 core

in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 uColor;

out vec4 fragColor;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 normal = normalize(vNormal);
    float diffuse = max(dot(normal, lightDir), 0.3);
    vec3 shaded = uColor * diffuse;
    fragColor = vec4(shaded, 1.0);
}