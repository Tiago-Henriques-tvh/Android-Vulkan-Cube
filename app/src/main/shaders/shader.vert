#version 450

// Uniform buffer containing an MVP matrix.
// Currently the vulkan backend only sets the rotation matix required to handle device rotation.
layout(binding = 0) uniform UniformBufferObject {
    mat4 MVP;
} ubo;

vec2 positions[4] = vec2[](
    vec2(-0.5, -0.5), // Bottom-left
    vec2(0.5, -0.5),  // Bottom-right
    vec2(0.5, 0.5),   // Top-right
    vec2(-0.5, 0.5)   // Top-left
);

vec2 texCoords[4] = vec2[](
    vec2(0.0, 0.0), // Bottom-left
    vec2(1.0, 0.0), // Bottom-right
    vec2(1.0, 1.0), // Top-right
    vec2(0.0, 1.0)  // Top-left
);

layout(location = 0) out vec2 vTexCoords;

void main() {
    gl_Position = ubo.MVP * vec4(positions[gl_VertexIndex], 0.0, 1.0);
    vTexCoords = texCoords[gl_VertexIndex];
}
