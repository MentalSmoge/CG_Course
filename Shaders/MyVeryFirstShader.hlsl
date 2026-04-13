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
    float3 worldPos : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float4 lightSpacePos : TEXCOORD3;
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
cbuffer LightBuffer : register(b2)
{
    float3 lightDir;
    float padding1;

    float3 lightColor;
    float padding2;
    matrix lightViewProj;
};

cbuffer MaterialBuffer : register(b3)
{
    float3 ambient;
    float padding3;

    float3 diffuse;
    float padding4;

    float3 specular;
    float shininess;
};

cbuffer CameraBufferPS : register(b4)
{
    float3 cameraPos;
    float padding5;
};
Texture2D tex : register(t0);
SamplerState samp : register(s0);
// shadow map
Texture2D shadowMap : register(t1);
SamplerComparisonState shadowSampler : register(s1);
PS_IN VSMain(VS_IN input, uint vertexID : SV_VertexID)
{
    PS_IN output;

    float3 animatedPos = input.pos.xyz + modelOffset;

    float4 worldPos = mul(float4(animatedPos, 1.0), world);
    float4 viewPos = mul(worldPos, view);

    output.pos = mul(viewPos, proj);

    output.worldPos = worldPos.xyz;

    output.normal = normalize(mul(input.normal, (float3x3) world));

    // позици€ в пространстве света
    output.lightSpacePos = mul(worldPos, lightViewProj);

    output.col = input.col;
    output.uv = input.uv;
    return output;
}

// функци€ тени
float ShadowCalculation(float4 lightSpacePos, float3 N, float3 L)
{
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // перевод в [0,1]
    projCoords = projCoords * 0.5f + 0.5f;
    projCoords.y = 1.0f - projCoords.y;
    // если вне shadow map Ч не в тени
    if (projCoords.x < 0 || projCoords.x > 1 ||
        projCoords.y < 0 || projCoords.y > 1)
        return 1.0f;
    float bias = max(0.001f * (1.0f - dot(N, L)), 0.0005f);
    //float bias = 0.05f;

    float shadow = shadowMap.SampleCmpLevelZero(
        shadowSampler,
        projCoords.xy,
        projCoords.z - bias
    );

    return shadow;
}

float4 PSMain(PS_IN input) : SV_Target
{
    float3 N = normalize(input.normal);
    float3 L = normalize(-lightDir);

    float diff = max(dot(N, L), 0.0);

    float3 V = normalize(cameraPos - input.worldPos);

    float3 R = reflect(-L, N);

    float spec = pow(max(dot(V, R), 0.0), shininess);

    // тень
    float shadow = ShadowCalculation(input.lightSpacePos, N, L);

    float3 lighting =
        ambient +
        shadow * (diffuse * diff + specular * spec);

    float4 texColor = tex.Sample(samp, input.uv);
    //return float4(shadow, shadow, shadow, 1.0);
    
    return float4(lighting, 1.0f) * texColor;
}
struct VS_OUT_SHADOW
{
    float4 pos : SV_POSITION;
};
VS_OUT_SHADOW VSShadow(VS_IN input)
{
    VS_OUT_SHADOW o;
    float3 animatedPos = input.pos.xyz + modelOffset;
    float4 worldPos = mul(float4(animatedPos, 1.0), world);
    o.pos = mul(worldPos, lightViewProj);
    return o;
}

float PSShadow(VS_OUT_SHADOW input) : SV_Depth
{
    return input.pos.z / input.pos.w;
}