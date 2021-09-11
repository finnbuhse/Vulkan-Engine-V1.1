#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define PI 3.1415926535
#define PI_RECIPRICOL 0.31830988618

// PRECOMPILED CONSTANTS
#define MAX_PREFILTER_LOD 11

#define MAX_DIRECTIONAL_LIGHTS 500
#define MAX_POINT_LIGHTS 500
#define MAX_SPOT_LIGHTS 500
// ---------------------

#define GAMMA 2.2
#define EXPOSURE 1.0

layout (location = 0) in vec3 worldFragment;
layout (location = 1) in vec2 textureCoordinate;
layout (location = 2) in mat3 TBN;

layout (std140, set = 0, binding = 0) uniform Camera
{
	mat4 projectionMatrix;
	mat4 viewMatrix;
	vec3 position;
	//float exposure;
} camera;

layout (set = 0, binding = 1) uniform samplerCube irradianceMap;
layout (set = 0, binding = 2) uniform samplerCube prefilterMap;
layout (set = 0, binding = 3) uniform sampler2D brdfLUT;

layout (set = 1, binding = 1) uniform sampler2D albedoTexture;
layout (set = 1, binding = 2) uniform sampler2D normalTexture;
layout (set = 1, binding = 3) uniform sampler2D roughnessTexture;
layout (set = 1, binding = 4) uniform sampler2D metalnessTexture;
layout (set = 1, binding = 5) uniform sampler2D ambientOcclusionTexture;

layout (std140, set = 2, binding = 0) uniform DirectionalLight
{
    vec3 colour;
    vec3 direction;
} directionalLight;

layout (location = 0) out vec3 colour;

float D(float NdotH, float r) // Trowbridge-Reitz GGX
{
    float rPow4 = r * r * r * r;
    float k = NdotH * NdotH * (rPow4 - 1.0) + 1.0;
    return rPow4 / (PI * k * k);
}

float G0(float dot, float k) // Schlick GGX
{
    return dot / (dot * (1.0 - k) + k);
}

float G(float NdotL, float NdotV, float r) // Smith
{
    float k0 = r + 1.0;
    float k = k0 * k0 * 0.125;
    return G0(NdotL, k) * G0(NdotV, k);
}

float G_IBL(float NdotL, float NdotV, float r) // Smith
{
    float k = r * r * 0.5;
    return G0(NdotL, k) * G0(NdotV, k);
}

vec3 F(float dot, vec3 F0) // Schlick using spherical gaussian approximation mentioned by Epic Games
{
    return F0 + (1.0 - F0) * pow(2, (-5.55473 * dot - 6.98316) * dot);
}

vec3 FRoughness(float dot, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - dot, 0.0), 5.0);
}  

vec3 BRDF(vec3 albedo, vec3 normal, float roughness, float metalness, vec3 reflectivity, vec3 radiance, vec3 fragmentToLight, vec3 fragmentToView) // Cook-Torrance
{
    vec3 h = normalize(fragmentToLight + fragmentToView);
    float NdotL = max(dot(normal, fragmentToLight), 0.0);
    float NdotV = max(dot(normal, fragmentToView), 0.0);
    float D = D(max(dot(normal, h), 0.0), roughness);
    vec3 F = F(max(dot(fragmentToView, h), 0.0), reflectivity);
    float G = G(NdotL, NdotV, roughness);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metalness);

    return (kD * albedo * PI_RECIPRICOL + D * F * G / (4 * NdotL * NdotV + 0.001)) * radiance * NdotL;
}

void main()
{
    vec3 albedo = texture(albedoTexture, textureCoordinate).rgb;
    vec3 normal = normalize(TBN * normalize(texture(normalTexture, textureCoordinate).xyz * 2.0 - vec3(1.0)));
    float roughness = texture(roughnessTexture, textureCoordinate).r;
    float metalness = texture(metalnessTexture, textureCoordinate).r;
    float ambientOcclusion = texture(ambientOcclusionTexture, textureCoordinate).r;

    vec3 reflectivity = mix(vec3(0.04), albedo, metalness);

    vec3 fragmentToView = normalize(camera.position - worldFragment);

    colour = vec3(0.0);//BRDF(albedo, normal, roughness, metalness, reflectivity, directionalLight.colour, -directionalLight.direction, fragmentToView);

    vec3 diffuse = albedo * texture(irradianceMap, normal).rgb;

    vec3 R = reflect(-fragmentToView, normal);
    vec3 prefilteredColour = textureLod(prefilterMap, R, roughness * MAX_PREFILTER_LOD).rgb;
    
    float NdotV = max(dot(normal, fragmentToView), 0.0);

    vec3 fresnel = /*F(NdotV, reflectivity)*/FRoughness(NdotV, reflectivity, roughness);
    vec3 kD = (vec3(1.0) - fresnel) * (1.0 - metalness);

    //vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;

    vec3 specular = prefilteredColour * fresnel;/*brdf.x + brdf.y*/; // Makes strange dot?

    // ambient
    colour += (kD * diffuse + specular) * ambientOcclusion;
	
    colour = 1.0 - exp(-colour * EXPOSURE); // Exposure tone mapping
    colour = pow(colour, vec3(1.0 / GAMMA)); // Gamma correction
}

