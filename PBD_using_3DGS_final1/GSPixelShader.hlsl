#include "Common.hlsli"

Texture2D g_texture0 : register(t0);
TextureCube g_diffuseCube : register(t1);
TextureCube g_specularCube : register(t2);
SamplerState g_sampler : register(s0);

cbuffer BasicPixelConstantBuffer : register(b0) {
    float3 eyeWorld;
    bool useTexture;
    Material material;
    Light light[MAX_LIGHTS];
};

// Schlick approximation: Eq. 9.17 in "Real-Time Rendering 4th Ed."
// fresnelR0는 물질의 고유 성질
// Water : (0.02, 0.02, 0.02)
// Glass : (0.08, 0.08, 0.08)
// Plastic : (0.05, 0.05, 0.05)
// Gold: (1.0, 0.71, 0.29)
// Silver: (0.95, 0.93, 0.88)
// Copper: (0.95, 0.64, 0.54)
float3 SchlickFresnel(float3 fresnelR0, float3 normal, float3 toEye) {
    // 참고 자료들
    // THE SCHLICK FRESNEL APPROXIMATION by Zander Majercik, NVIDIA
    // http://psgraphics.blogspot.com/2020/03/fresnel-equations-schlick-approximation.html

    float normalDotView = saturate(dot(normal, toEye));

    float f0 = 1.0f - normalDotView; // 90도이면 f0 = 1, 0도이면 f0 = 0

    // 1.0 보다 작은 값은 여러 번 곱하면 더 작은 값이 됩니다.
    // 0도 -> f0 = 0 -> fresnelR0 반환
    // 90도 -> f0 = 1.0 -> float3(1.0) 반환
    // 0도에 가까운 가장자리는 Specular 색상, 90도에 가까운 안쪽은 고유
    // 색상(fresnelR0)
    return fresnelR0 + (1.0f - fresnelR0) * pow(f0, 5.0);
}

float4 main(GSPixelShaderInput input) : SV_TARGET {
    float3 toEye = normalize(eyeWorld - input.posWorld);
    float2 uv = input.texcoord;

    // if (dot(toEye, input.normalWorld) < 0.0f)
    //    discard;

    // 타원형 Gaussian
    float2 sigma = float2(0.7f, 0.7f); // sharpness 조절용
    float2 uvNorm = uv / sigma;
    float r2 = dot(uvNorm, uvNorm);
    clip(1.0f - r2); // r2 > 1 이면 1 - r2 < 0 → discard

    float3 color = float3(0.0, 0.0, 0.0);

    int i = 0;

    [unroll] for (i = 0; i < NUM_DIR_LIGHTS; ++i) {
        color += ComputeDirectionalLight(light[i], material, input.normalWorld,
                                         toEye);
    }

    [unroll] for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS;
                  ++i) {
        color += ComputePointLight(light[i], material, input.posWorld,
                                   input.normalWorld, toEye);
    }

    [unroll] for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS;
                  i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS;
                  ++i) {
        color += ComputeSpotLight(light[i], material, input.posWorld,
                                  input.normalWorld, toEye);
    }


    // normal로 가져오는 방법 (기존 방법)
    //float4 diffuse = g_diffuseCube.Sample(g_sampler, input.normalWorld);
    //float4 specular =
    //    g_specularCube.Sample(g_sampler, reflect(-toEye, input.normalWorld));

    // World Position으로 가져오는 방법
    float4 diffuse = g_diffuseCube.Sample(g_sampler, input.posWorld);
    float4 specular =
        g_specularCube.Sample(g_sampler, reflect(-toEye, input.posWorld));

    diffuse *= float4(material.diffuse, 1.0);
    specular *= pow(abs(specular.r + specular.g + specular.b) / 3.0,
                    material.shininess);
    specular *= float4(material.specular, 1.0);

    // 참고: https://www.shadertoy.com/view/lscBW4
    //float3 f = SchlickFresnel(material.fresnelR0, input.normalWorld, toEye);
    //specular.xyz *= f;

    // texcoord 대신 World Position으로 가져옴
    if (useTexture) {
        diffuse *= g_texture0.Sample(g_sampler, input.posWorld);
        // Specular texture를 별도로 사용할 수도 있습니다.
    }

    return diffuse + specular;
}
