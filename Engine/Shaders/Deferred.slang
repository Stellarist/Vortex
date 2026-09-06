import "Common";

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;
[[vk::binding(1, 0)]] StructuredBuffer<LightData> scene_lights;

[[vk::binding(0, 1)]] Texture2D<float4> albedo_metallic_gbuffer;
[[vk::binding(1, 1)]] Texture2D<float4> normal_roughness_gbuffer;
[[vk::binding(2, 1)]] Texture2D<float4> emissive_ao_gbuffer;
[[vk::binding(3, 1)]] Texture2D<float> scene_depth;
[[vk::binding(4, 1)]] SamplerState gbuffer_sampler;


[[vk::binding(0, 2)]] ConstantBuffer<ShadowData> shadow_data;
[[vk::binding(1, 2)]] Texture2D<float> directional_shadow_map;
[[vk::binding(2, 2)]] SamplerState shadow_sampler;

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

[shader("vertex")] FScreenOutput vertexMain(uint vertex_id : SV_VertexID) {
	float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
	FScreenOutput output;
	output.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
	output.uv = uv;
	return output;
}

    [shader("pixel")] float4 pixelMain(FScreenOutput input) : SV_Target
{
	float depth = scene_depth.SampleLevel(gbuffer_sampler, input.uv, 0.0);
	if (depth >= 0.999999)
		return float4(0.0, 0.0, 0.0, 1.0);

	float4 world_h = mul(scene.camera.inverse_view_projection,
	    float4(input.uv * 2.0 - 1.0, depth, 1.0));
	float3 world_pos = world_h.xyz / max(abs(world_h.w), 0.000001);
	float4 albedo_metallic = albedo_metallic_gbuffer.SampleLevel(gbuffer_sampler, input.uv, 0.0);
	float4 normal_roughness = normal_roughness_gbuffer.SampleLevel(gbuffer_sampler, input.uv, 0.0);
	float4 emissive_ao = emissive_ao_gbuffer.SampleLevel(gbuffer_sampler, input.uv, 0.0);

	float3 N = normalize(normal_roughness.xyz);
	float3 V = normalize(scene.camera.position.xyz - world_pos);
	float3 albedo = albedo_metallic.rgb;
	float metallic = albedo_metallic.a;
	float roughness = max(normal_roughness.a, 0.045);

	float3 Lo = float3(0.0);
	bool directional_shadow_used = false;
	for (uint light_index = 0; light_index < scene.light_count; ++light_index) {
		LightData light_data = scene_lights[light_index];
		LightSample light = sampleLight(light_data, world_pos);
		float3 brdf = BRDF_Cook(light.direction, V, N, albedo, metallic, roughness);
		float NdotL = max(dot(N, light.direction), 0.0);
		float visibility = 1.0;
		if (!directional_shadow_used && uint(light_data.params.w) == 0) {
			visibility = sampleDirectionalShadow(world_pos, N, light.direction);
			directional_shadow_used = true;
		}
		Lo += brdf * light.radiance * NdotL * visibility;
	}

	float3 ambient = scene.ambient_color.rgb * albedo * emissive_ao.a;
	return float4(emissive_ao.rgb + ambient + Lo, 1.0);
}
