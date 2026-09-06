import "Common";

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;
[[vk::binding(1, 0)]] StructuredBuffer<LightData> scene_lights;

[[vk::binding(0, 2)]] ConstantBuffer<ObjectData> object;

[[vk::binding(0, 1)]] ConstantBuffer<MaterialData> material;
[[vk::binding(1, 1)]] Texture2D base_color_texture;
[[vk::binding(2, 1)]] Texture2D metallic_roughness_texture;
[[vk::binding(3, 1)]] SamplerState material_sampler;

[[vk::binding(0, 3)]] ConstantBuffer<ShadowData> shadow_data;
[[vk::binding(1, 3)]] Texture2D<float> directional_shadow_map;
[[vk::binding(2, 3)]] SamplerState shadow_sampler;

float sampleDirectionalShadow(float3 world_position, float3 normal, float3 light_direction)
{
	if (shadow_data.params.z < 0.5)
		return 1.0;

	float4 clip = mul(shadow_data.light_view_projection, float4(world_position, 1.0));
	if (clip.w <= 0.0)
		return 1.0;
	float3 projected = clip.xyz / clip.w;
	float2 uv = projected.xy * 0.5 + 0.5;
	if (projected.z <= 0.0 || projected.z >= 1.0 || any(uv < 0.0) || any(uv > 1.0))
		return 1.0;

	float bias = max(shadow_data.params.x * (1.0 - max(dot(normal, light_direction), 0.0)),
	    shadow_data.params.x * 0.25);
	float visibility = 0.0;
	for (int y = -1; y <= 1; ++y)
		for (int x = -1; x <= 1; ++x) {
			float sampled_depth = directional_shadow_map.SampleLevel(
			    shadow_sampler, uv + float2(x, y) * shadow_data.params.y, 0.0);
			visibility += projected.z - bias <= sampled_depth ? 1.0 : 0.0;
		}
	return visibility / 9.0;
}


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


    [shader("pixel")] ForwardOutput pixelMain(VSOutput input)
{
	ForwardOutput output;
	float3 N = normalize(input.normal);
	float3 V = normalize(scene.camera.position.xyz - input.world_pos);

	float4 base_color = base_color_texture.Sample(material_sampler, input.uv);
	float4 metallic_roughness = metallic_roughness_texture.Sample(material_sampler, input.uv);

	float4 surface_color = base_color * material.base_color * input.color;
	if (material.alpha_mode == 1 && surface_color.a < material.alpha_cutoff)
		discard;

	float3 albedo = surface_color.rgb;
	float metallic = metallic_roughness.b * material.metallic;
	float roughness = max(metallic_roughness.g * material.roughness, 0.045);
	float opacity = surface_color.a;

	float3 Lo = float3(0.0);
	bool directional_shadow_used = false;
	for (uint light_index = 0; light_index < scene.light_count; ++light_index) {
		LightData light_data = scene_lights[light_index];
		LightSample light = sampleLight(light_data, input.world_pos);

		float3 brdf = BRDF_Cook(light.direction, V, N, albedo, metallic, roughness);
		float NdotL = max(dot(N, light.direction), 0.0);

		float visibility = 1.0;
		if (!directional_shadow_used && uint(light_data.params.w) == 0) {
			visibility = sampleDirectionalShadow(input.world_pos, N, light.direction);
			directional_shadow_used = true;
		}
		Lo += brdf * light.radiance * NdotL * visibility;
	}

	float3 ambient = scene.ambient_color.rgb * albedo;
	float3 color = ambient + Lo;

	output.color = float4(color, opacity);
	return output;
}
