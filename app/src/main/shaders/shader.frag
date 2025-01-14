#version 450

layout(location = 2) in vec3 fragColor;// Color from vertex shader
layout(location = 3) in vec3 lightPos;// Light position from vertex shader
layout(location = 4) in vec3 lightColor;// Light color from vertex shader

layout(location = 0) out vec4 outColor;

void main() {
    // Calculate the direction from the fragment to the light source
    vec3 lightDir = normalize(lightPos + gl_FragCoord.xyz);

    // Lambertian reflectance: Diffuse reflection
    float diff = max(dot(normalize(fragColor), lightDir), 0.0);

    // Combine diffuse reflection with light color and fragment color
    vec3 finalColor = lightColor * diff * fragColor;

    // Apply the calculated color to the fragment
    outColor = vec4(finalColor, 1.0);
}
