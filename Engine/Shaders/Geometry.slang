import "Common";

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;
[[vk::binding(1, 0)]] StructuredBuffer<LightData> scene_lights;

[[vk::binding(0, 2)]] ConstantBuffer<ObjectData> object;

[[vk::binding(0, 1)]] ConstantBuffer<MaterialData> material;
[[vk::binding(1, 1)]] Texture2D base_color_texture;
[[vk::binding(2, 1)]] Texture2D metallic_roughness_texture;
[[vk::binding(3, 1)]] SamplerState material_sampler;


[shader("vertex")] VSOutput vertexMain(VSInput input) {
	VSOutput output;
	float4 world_position = mul(object.model, float4(input.pos, 1.0));

	output.position = mul(scene.camera.projection, mul(scene.camera.view, world_position));
	output.world_pos = world_position.xyz;
	output.normal = mul((float3x3) (object.normal_matrix), input.normal);
	output.uv = input.uv;
	output.color = input.color;

	return output;
}


    [shader("pixel")] GBufferOutput pixelMain(VSOutput input)
{
	GBufferOutput output;

	float4 base_color = base_color_texture.Sample(material_sampler, input.uv) * material.base_color * input.color;
	if (material.alpha_mode == 1 && base_color.a < material.alpha_cutoff)
		discard;
	float4 metallic_roughness = metallic_roughness_texture.Sample(material_sampler, input.uv);

	output.albedo_metallic = float4(base_color.rgb, metallic_roughness.b * material.metallic);
	output.normal_roughness = float4(
	    normalize(input.normal), max(metallic_roughness.g * material.roughness, 0.045));
	output.emissive_ao = material.emissive_ao;

	return output;
}
