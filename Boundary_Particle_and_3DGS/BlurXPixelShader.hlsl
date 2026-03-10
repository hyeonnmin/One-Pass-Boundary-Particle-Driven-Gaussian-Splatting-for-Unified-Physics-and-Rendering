Texture2D g_texture0 : register(t0);
SamplerState g_sampler : register(s0);

static const float weights[5] = { 0.0545, 0.2442, 0.4026, 0.2442, 0.0545 };

cbuffer SamplingPixelConstantData : register(b0)
{
    float dx;
    float dy;
    float threshold;
    float strength;
    float4 options;
};

struct SamplingPixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(SamplingPixelShaderInput input) : SV_TARGET
{
    // Compute Shader X
    
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}