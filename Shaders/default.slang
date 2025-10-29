struct VSInput {
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

struct VSOutput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv: TEXCOORD;
    float4 color : COLOR;
};

struct SceneData{
    float4x4 view;
    float4x4 projection;
    float4 ambient_color;
};

struct MaterialData{
    float4 base_color;
    float metallic;
    float roughness;
    float padding1;
    float padding2;
};

struct ObjectData{
    float4x4 model;
};

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;
[[vk::binding(0, 1)]] ConstantBuffer<MaterialData> material;
[[vk::binding(0, 2)]] ConstantBuffer<ObjectData> object;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.position = mul(scene.projection,
                        mul(scene.view,
                        mul(object.model, 
                        float4(input.pos, 1.0))));

    // Transform normal to world space
    // output.normal = mul((float3x3)(inverse(object.model)), input.normal);
    output.normal = mul((float3x3)(object.model), input.normal);


    output.uv = input.uv;
    output.color = input.color;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) 
{
    // float4 tex_color = texture(texSampler, fragTexCoord);
    float4 tex_color = float4(1.0);
    
    float3 light_dir = normalize(float3(1.0, 1.0, 1.0));
    float3 normal = normalize(input.normal);
    float diff = max(dot(normal, light_dir), 0.0) * 0.8;
    
    float3 ambient = float3(0.1);
    float3 diffuse = diff * float3(1.0);
    float3 lighting = ambient + diffuse;
    
    float4 out_color = tex_color * material.base_color * input.color * float4(lighting, 1.0);

    return out_color;
}
