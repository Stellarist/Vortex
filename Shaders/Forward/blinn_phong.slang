import "../common";

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> scene;

[[vk::binding(0, 2)]] ConstantBuffer<ObjectData> object;

[[vk::binding(0, 1)]] ConstantBuffer<MaterialData> material;
[[vk::binding(1, 1)]] Sampler2D                    base_color_texture;

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
	float4 tex_color = base_color_texture.Sample(input.uv);

	float3 N = normalize(input.normal);
	float3 V = normalize(scene.camera_position.xyz - input.world_pos);
	float  opacity = tex_color.a;

	float3 ambient = scene.ambient_color.rgb * scene.ambient_color.a;
	float3 lighting = ambient;

	for (uint i = 0; i < scene.light_count; ++i) {
		LightSample light = sampleLight(scene.lights[i], input.world_pos);

		float  diff = max(dot(N, light.direction), 0.0);
		float3 diffuse_light = diff * light.radiance;

		float3 H = normalize(light.direction + V);
		float  spec = pow(max(dot(N, H), 0.0), 32.0);
		float3 specular_light = spec * light.radiance;

		lighting += diffuse_light + specular_light;
	}

	float3 color = lighting * tex_color.rgb;

	return float4(ACESFilm(color), opacity);
}
