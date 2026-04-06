#pragma shader_model 5_0
struct VS_IN
{
    float4 pos : POSITION0;
    float3 normal : NORMAL0;
    float2 uv : TEXCOORD0;
    float4 col : COLOR0;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv : TEXCOORD;
};

cbuffer VSConstants : register(b0)
{
    matrix view;
    matrix proj;
    matrix world;
    float time;
    float3 modelOffset;
};
cbuffer PSConstants : register(b1)
{
    int hasTexture;
};
Texture2D tex : register(t0);
SamplerState samp : register(s0);
PS_IN VSMain(VS_IN input, uint vertexID : SV_VertexID)
{
    PS_IN output;

    float frequency = 2.0;
    float amplitude = 0.02;
    float phase = vertexID * 0.1;

    float offset = sin(time * frequency + phase) * amplitude;

    float3 animatedPos = input.pos.xyz + input.normal * offset + modelOffset;

    float4 worldPos = mul(float4(animatedPos, 1.0), world);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, proj);

    output.col = input.col;
    output.uv = input.uv;
    return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
        return tex.Sample(samp, input.uv);
}