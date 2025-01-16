#version 450

layout(location = 0) in vec3 fragColor;// Color from vertex shader
layout(location = 1) in vec3 lightPos;// Light position from vertex shader
layout(location = 2) in vec3 lightColor;// Light color from vertex shader
layout(location = 3) in vec3 fragPos;// Fragment position from vertex shader

layout(location = 4) in vec2 vTexCoords;
layout(set = 2, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;// Final color of the fragment

// Simplified lighting parameters
const vec3 ambientColor = vec3(0.8, 0.8, 0.8);// Ambient light color

void main() {
    // Set final color of the fragment
    // Apply texture if valid texture coordinates are provided
    if (vTexCoords.x < 0.0 || vTexCoords.y < 0.0) {
        // Calculate the light direction (from fragment to light)
        vec3 lightDir = normalize(lightPos - fragPos);

        // Basic Lambertian Diffuse Reflection (without normal)
        float diff = max(dot(lightDir, vec3(0.0, 0.0, 1.0)), 0.0);// Assuming a "flat" surface facing up

        // Combine ambient, diffuse, and the effect of light color
        vec3 ambient = ambientColor * fragColor;
        vec3 diffuse = diff * lightColor * fragColor;

        // Final color calculation
        vec3 finalColor = ambient + diffuse;

        // If no valid texture coordinates, use the final color only
        outColor = vec4(finalColor, 1.0);
    } else {
        // Sample the texture and combine it with the final color
        outColor = texture(textureSampler, vTexCoords);
        // outColor = vec4(vTexCoords, 0.0, 1.0);
    }
}
