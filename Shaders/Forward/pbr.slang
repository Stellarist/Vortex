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

    [shader("fragment")] float4 fragmentMain(VSOutput input)
{
	float3 N = normalize(input.normal);
	float3 V = normalize(scene.camera_position.xyz - input.world_pos);

	float4 base_color = base_color_texture.Sample(input.uv);
	float4 metallic_roughness = metallic_roughness_texture.Sample(input.uv);

	float3 albedo = base_color.rgb * material.base_color.rgb * input.color.rgb;
	float  metallic = metallic_roughness.b * material.metallic;
	float  roughness = metallic_roughness.g * material.roughness;
	float  opacity = base_color.a;

	float3 Lo = float3(0.0);
	for (uint i = 0; i < scene.light_count; i++) {
		LightSample light = sampleLight(scene.lights[i], input.world_pos);

		float3 brdf = BRDF_CT(light.direction, V, N, albedo, metallic, roughness);
		float  NdotL = max(dot(N, light.direction), 0.0);

		Lo += brdf * light.radiance * NdotL;
	}

	float3 ambient = scene.ambient_color.rgb * albedo;
	float3 color = ambient + Lo;

	return float4(ACESFilm(color), opacity);
}
