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
PS_IN VSMain(VS_IN input, uint vertexID : SV_VertexID)
{
    PS_IN output;

    float frequency = 2.0;
    float amplitude = 0.02;
    float phase = vertexID * 0.1;
    
    float offset = sin(time * frequency + phase) * amplitude;

    float3 animatedPos = input.pos.xyz + modelOffset;

    float4 worldPos = mul(float4(animatedPos, 1.0), world);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, proj);
    
    output.worldPos = worldPos.xyz;
    
    output.normal = normalize(mul(input.normal, (float3x3)world));

    output.col = input.col;
    output.uv = input.uv;
    return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
    float3 N = normalize(input.normal);
    float3 L = normalize(-lightDir);
    
    float diff = max(dot(N, L), 0.0);
    
    float3 V = normalize(cameraPos - input.worldPos);
    
    float3 R = reflect(-L, N);
    
    float spec = pow(max(dot(V, R), 0.0), shininess);

    float3 color = ambient +
                   diffuse * diff +
                   specular * spec;

    float4 texColor = tex.Sample(samp, input.uv);

    return float4(color, 1.0) * texColor;
}