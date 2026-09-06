import "Common";

[[vk::binding(0, 0)]] ConstantBuffer<ShadowData> shadow_data;
[[vk::binding(0, 1)]] ConstantBuffer<ObjectData> object;

struct ShadowVertexInput {
	float3 pos : POSITION;
};

[shader("vertex")] float4 vertexMain(ShadowVertexInput input) : SV_Position
{
	float4 world_position = mul(object.model, float4(input.pos, 1.0));
	return mul(shadow_data.light_view_projection, world_position);
}

[shader("pixel")] void pixelMain() {}
