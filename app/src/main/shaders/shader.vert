#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(set = 1, binding = 0) uniform LightUBO {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
} lightData;

layout(location = 0) in vec3 inPos;  // Vertex position
layout(location = 1) in vec3 inColor;  // Vertex color
layout(location = 2) in vec2 inTexCoord;  // Texture coordinates

layout(location = 0) out vec3 fragColor;  // Output color to fragment shader
layout(location = 1) out vec3 lightPos;  // Output light position to fragment shader
layout(location = 2) out vec3 lightColor;  // Output light color to fragment shader
layout(location = 3) out vec3 fragPos;  // Output fragment position to fragment shader

layout(location = 4) out vec2 fragTexCoord;

void main() {
    // Transform vertex position to camera space
    vec4 viewPos = ubo.view * ubo.model * vec4(inPos, 1.0);
    gl_Position = ubo.proj * viewPos;
    fragTexCoord = inTexCoord;            // Pass tex coords to fragment shader

    // Pass data to the fragment shader
    fragColor = inColor;
    fragPos = viewPos.xyz;

    // Pass light data to the fragment shader
    lightPos = lightData.position;
    lightColor = lightData.color;
}
