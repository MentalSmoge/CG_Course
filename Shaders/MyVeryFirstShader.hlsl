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
    matrix normalMatrix;
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

cbuffer ShadowBuffer : register(b5)
{
    matrix lightViewProj;
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

Texture2D shadowMap : register(t1);
SamplerComparisonState shadowSampler : register(s1);

static const float SHADOW_BIAS = 0.005f;

PS_IN VSMain(VS_IN input, uint vertexID : SV_VertexID)
{
    PS_IN output;

    float3 animatedPos = input.pos.xyz + modelOffset;

    float4 worldPos = mul(float4(animatedPos, 1.0), world);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, proj);
    
    output.lightSpacePos = mul(worldPos, lightViewProj);

    output.worldPos = worldPos.xyz;
    output.normal = normalize(mul(float4(input.normal, 0.0f), normalMatrix).xyz);

    output.col = input.col;
    output.uv = input.uv;
    return output;
}

float CalcShadowFactor(float4 lightSpacePos)
{
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    
    float2 shadowTexCoord;
    shadowTexCoord.x = 0.5f + (projCoords.x * 0.5f);
    shadowTexCoord.y = 0.5f - (projCoords.y * 0.5f);
    
    if (shadowTexCoord.x < 0.0f || shadowTexCoord.x > 1.0f ||
        shadowTexCoord.y < 0.0f || shadowTexCoord.y > 1.0f ||
        projCoords.z < 0.0f || projCoords.z > 1.0f)
    {
        return 1.0f;
    }
    float bias = 0.005f;
    float currentDepth = projCoords.z - bias;
    float shadow = shadowMap.SampleCmpLevelZero(shadowSampler, shadowTexCoord, currentDepth);
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
    
    float shadowFactor = CalcShadowFactor(input.lightSpacePos);

    float3 color = ambient +
                   diffuse * diff * shadowFactor +
                   specular * spec * shadowFactor;

    float4 texColor = tex.Sample(samp, input.uv);
    
    return float4(color, 1.0) * texColor;
}