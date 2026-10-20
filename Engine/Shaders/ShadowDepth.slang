import "Common";

[[vk::binding(0, 0)]] ConstantBuffer<ShadowPassConstants> shadow : register(b0, space0);
[[vk::binding(0, 1)]] ConstantBuffer<MaterialConstants> material : register(b0, space1);
[[vk::binding(1, 1)]] Texture2D base_color_texture : register(t1, space1);
[[vk::binding(2, 1)]] Texture2D metallic_roughness_texture : register(t2, space1);
[[vk::binding(3, 1)]] SamplerState material_sampler : register(s3, space1);
[[vk::push_constant]] ConstantBuffer<PrimitiveConstants> primitive : register(b0, space255);

struct ShadowVertexInput {
	float3 pos : POSITION;
	float2 uv : TEXCOORD;
	float4 color : COLOR;
};

struct ShadowVertexOutput {
	float4 position : SV_Position;
	float2 uv : TEXCOORD;
	float4 color : COLOR;
};

[shader("vertex")] ShadowVertexOutput vertexMain(ShadowVertexInput input)
{
	float4 world_position = mul(primitive.model, float4(input.pos, 1.0));
	ShadowVertexOutput output;
	output.position = toRasterPosition(mul(shadow.light_view_projection, world_position));
	output.uv = input.uv;
	output.color = input.color;
	return output;
}

[shader("pixel")] void pixelMain(ShadowVertexOutput input)
{
	if (material.alpha_mode == 1) {
		float alpha = base_color_texture.Sample(material_sampler, input.uv).a *
			material.base_color.a * input.color.a;
		if (alpha < material.alpha_cutoff)
			discard;
	}
}
