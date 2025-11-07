import "../common";

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;

[[vk::binding(0, 3)]] Sampler2D position_gbuffer;
[[vk::binding(1, 3)]] Sampler2D normal_gbuffer;
[[vk::binding(2, 3)]] Sampler2D albedo_gbuffer;
[[vk::binding(3, 3)]] Sampler2D metallic_gbuffer;
[[vk::binding(4, 3)]] Sampler2D roughness_gbuffer;

[shader("vertex")] FScreenOutput vertexMain(uint vertexID : SV_VertexID) {
	FScreenOutput output;

	float2 uv = float2(vertexID & 2, (vertexID << 1) & 2);
	float2 pos = uv * 2.0 - 1.0;

	output.position = float4(pos, 0.0, 1.0);
	output.uv = uv;

	return output;
}

    [shader("fragment")] float4 fragmentMain(FScreenOutput input) : SV_Target
{
	float3 world_pos = position_gbuffer.Sample(input.uv).rgb;
	float3 N = normalize(normal_gbuffer.Sample(input.uv).rgb);
	float3 V = normalize(scene.camera_position.xyz - world_pos);

	float3 albedo = albedo_gbuffer.Sample(input.uv).rgb;
	float  opacity = albedo_gbuffer.Sample(input.uv).a;

	float3 ambient = scene.ambient_color.rgb * scene.ambient_color.a;
	float3 lighting = ambient;

	for (uint i = 0; i < scene.light_count; ++i) {
		LightSample light = sampleLight(scene.lights[i], world_pos);

		float  diff = max(dot(N, light.direction), 0.0);
		float3 diffuse_light = diff * light.radiance;

		float3 H = normalize(light.direction + V);
		float  spec = pow(max(dot(N, H), 0.0), 32.0);
		float3 specular_light = spec * light.radiance;

		lighting += diffuse_light + specular_light;
	}

	float3 color = lighting * albedo;

	return float4(ACESFilm(color), opacity);
}
