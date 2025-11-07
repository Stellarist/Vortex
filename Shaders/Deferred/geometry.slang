import "../common";

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;

[[vk::binding(0, 2)]] ConstantBuffer<ObjectData> object;

[[vk::binding(0, 1)]] ConstantBuffer<MaterialData> material;
[[vk::binding(1, 1)]] Sampler2D                    base_color_texture;
[[vk::binding(2, 1)]] Sampler2D                    metallic_roughness_texture;

[shader("vertex")] VSOutput vertexMain(VSInput input) {
	VSOutput output;
	float4   world_position = mul(object.model, float4(input.pos, 1.0));

	output.position = mul(scene.projection, mul(scene.view, world_position));
	output.world_pos = world_position.xyz;
	output.normal = mul((float3x3) (object.model), input.normal);
	output.uv = input.uv;
	output.color = input.color;

	return output;
}

    [shader("fragment")] GBufferOutput fragmentMain(VSOutput input)
{
	GBufferOutput output;

	float4 base_color = base_color_texture.Sample(input.uv);
	float4 metallic_roughness = metallic_roughness_texture.Sample(input.uv);

	output.position = float4(input.world_pos, 1.0);
	output.normal = float4(normalize(input.normal), 1.0);

	output.albedo = float4(base_color.rgb * material.base_color.rgb * input.color.rgb, base_color.a);
	output.metallic = metallic_roughness.b * material.metallic;
	output.roughness = metallic_roughness.g * material.roughness;

	return output;
}
