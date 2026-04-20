cbuffer WorldBuffer : register(b0)
{
    matrix world;
};

cbuffer ShadowConstants : register(b1)
{
    matrix lightViewProj;
};

struct VS_IN
{
    float4 pos : POSITION0;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;
    float4 worldPos = mul(float4(input.pos.xyz, 1.0), world);
    output.pos = mul(worldPos, lightViewProj);
    return output;
}
float4 PSMain(PS_IN input) : SV_Target
{
    // Заглушка - пиксельный шейдер не используется при рендеринге теней,
    // но должен существовать для компиляции
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}