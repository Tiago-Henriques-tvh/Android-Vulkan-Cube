#version 450

struct Light {
    vec3 position;// Light position
    vec3 direction;// Light direction (for directional light)
    vec3 color;// Light color (RGB)
    float intensity;// Light intensity
    float constant;// Point light attenuation (constant)
    float linear;// Point light attenuation (linear)
    float quadratic;// Point light attenuation (quadratic)
};

layout(location = 0) in vec3 inPos;// Vertex position
layout(location = 1) in vec3 inColor;// Vertex color

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    Light light;// Light struct passed from UBO
} ubo;

layout(location = 2) out vec3 fragColor;// Output color to fragment shader
layout(location = 3) out vec3 lightPos;// Output light position to fragment shader
layout(location = 4) out vec3 lightColor;// Output light color to fragment shader

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0);
    fragColor = inColor;

    // Pass light data to the fragment shader
    lightPos = ubo.light.position;
    lightColor = ubo.light.color;
}