struct VS_IN
{
    float4 pos : POSITION0;
    float3 normal : NORMAL0;
    float4 col : COLOR0;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

cbuffer VSConstants : register(b0)
{
    matrix view;
    matrix proj;
    matrix world;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output = (PS_IN) 0;

    output.pos = mul(input.pos, world);
    output.pos = mul(output.pos, view);
    output.pos = mul(output.pos, proj);
    output.col = input.col;

    return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
    return input.col;
}