#pragma shader_model 5_0

#define MAX_POINT_LIGHTS 8

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

struct PointLight
{
    float3 Position;
    float Range;
    float3 Color;
    float Intensity;
    uint Enabled;
    float3 Padding;
};

cbuffer PointLightCB : register(b6)
{
    PointLight pointLights[MAX_POINT_LIGHTS];
    uint numPointLights;
    float3 padding6;
};
cbuffer ShadowTintParams : register(b7)
{
    float maxShadowDist;
    float shadowTintStrength;
    float2 padding7;
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

Texture2D shadowMap : register(t1);
Texture2D paletteTex : register(t2);
SamplerComparisonState shadowSampler : register(s1);

// Функция расчёта точечного освещения
float3 CalcPointLight(float3 worldPos, float3 normal, float3 viewDir, PointLight light, float shininess)
{
    if (light.Enabled == 0)
        return float3(0, 0, 0);
    
    float3 lightVec = light.Position - worldPos;
    float distance = length(lightVec);
    if (distance > light.Range)
        return float3(0, 0, 0);
    
    float attenuation = 1.0 - smoothstep(0.0, light.Range, distance);
    attenuation *= attenuation;
    
    float3 L = lightVec / distance;
    float3 H = normalize(L + viewDir);
    
    float diff = max(dot(normal, L), 0.0);
    float spec = pow(max(dot(normal, H), 0.0), shininess);
    
    float3 diffuseLighting = diffuse * diff;
    float3 specularLighting = specular * spec;
    
    return (diffuseLighting + specularLighting) * light.Color * light.Intensity * attenuation;
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

float4 PSMain(PS_IN input) : SV_Target
{
    float3 N = normalize(input.normal);
    float3 L = normalize(-lightDir);
    float3 V = normalize(cameraPos - input.worldPos);
    float3 R = reflect(-L, N);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(V, R), 0.0), shininess);
    
    float shadowFactor = CalcShadowFactor(input.lightSpacePos);

    // ====== Новый блок: цвет тени на основе расстояния ======
    float distToCamera = length(cameraPos - input.worldPos);
    // Нормализуем в диапазон 0..1, где 0 – близко, 1 – далеко
    float shadowU = saturate(distToCamera / maxShadowDist);
    
    // Сэмплируем палитру (используем тот же сэмплер samp, или отдельный, если нужно другое фильтрование)
    float3 shadowColor = paletteTex.Sample(samp, float2(shadowU, 0.5f)).rgb;
    
    // Маска тени: 0 = полностью в тени, 1 = нет тени
    float shadowAmount = 1.0 - shadowFactor;
    // =========================================================

    // Стандартное направленное освещение с учётом shadowFactor (останется чёрным в тени)
    float3 fullLightDir = (diffuse * diff + specular * spec) * lightColor;
    float3 directionalLight = fullLightDir * shadowFactor;
    
    // Добавляем цветную тень в затемнённые области
    directionalLight += fullLightDir * shadowColor * shadowAmount * shadowTintStrength;

    // Остальное без изменений
    float3 ambientLight = ambient * diffuse;
    
    float3 pointLighting = float3(0, 0, 0);
    for (uint i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; i++)
    {
        pointLighting += CalcPointLight(input.worldPos, N, V, pointLights[i], shininess);
    }
    
    float3 lighting = ambientLight + directionalLight + pointLighting;
    
    float4 texColor = tex.Sample(samp, input.uv);
    
    return float4(lighting, 1.0) * texColor * input.col;
}