static constexpr float PI = 3.14159265359;
static const float     LUMINOUS_EFFICIENCY = 683.0;

struct VSInput {
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
	float4 color : COLOR;
};

struct VSOutput {
	float4 position : SV_POSITION;
	float3 world_pos : POSITION0;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
	float4 color : COLOR;
};

struct LightData {
	float4 position;
	float4 direction;
	float4 color;
	float4 params;
};

struct ObjectData {
	float4x4 model;
};

struct MaterialData {
	float4 base_color;
	float  metallic;
	float  roughness;
	float  padding1;
	float  padding2;
};

struct SceneData {
	float4x4  view;
	float4x4  projection;
	float4    ambient_color;
	float4    camera_position;
	uint      light_count;
	float     padding1;
	float     padding2;
	float     padding3;
	LightData lights[16];
};

struct LightSample {
	float3 direction;
	float3 radiance;
};

struct GBufferOutput {
	float4 position : SV_Target0;
	float4 normal : SV_Target1;
	float4 albedo : SV_Target2;
	float  metallic : SV_Target3;
	float  roughness : SV_Target4;
};

struct FScreenOutput {
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

LightSample sampleLight(LightData light, float3 world_pos)
{
	LightSample result;

	float3 light_color = light.color.rgb;
	float  intensity = light.color.a;
	uint   light_type = uint(light.params.w);

	// Directional light (Intensity unit: lux (lm/m²))
	if (light_type == 0) {
		result.direction = normalize(-light.direction.xyz);

		// lux -> W/m²
		float irradiance = intensity / LUMINOUS_EFFICIENCY;
		result.radiance = light_color * irradiance;

		// Point light (Intensity unit: candela (lm/sr))
	} else if (light_type == 1) {
		float3 to_light = light.position.xyz - world_pos;
		float  distance = length(to_light);
		result.direction = normalize(to_light);

		// candela -> lux -> W/m²
		float illuminance = intensity / max(distance * distance, 0.0001);
		float irradiance = illuminance / LUMINOUS_EFFICIENCY;
		result.radiance = light_color * irradiance;

		// Spot light (Intensity unit: candela (lm/sr))
	} else {
		float3 to_light = light.position.xyz - world_pos;
		float  distance = length(to_light);
		result.direction = normalize(to_light);

		float theta = dot(result.direction, normalize(-light.direction.xyz));
		float inner_cone = cos(light.params.y);
		float outer_cone = cos(light.params.z);
		float epsilon = max(inner_cone - outer_cone, 0.0001);
		float cone_attenuation = clamp((theta - outer_cone) / epsilon, 0.0, 1.0);

		// candela -> lux -> W/m²
		float illuminance = intensity / max(distance * distance, 0.0001);
		float irradiance = illuminance / LUMINOUS_EFFICIENCY;
		result.radiance = light_color * irradiance * cone_attenuation;
	}

	return result;
}

// ACES Filmic Tone Mapping: HDR → LDR
float3 ACESFilm(float3 color)
{
	float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
	return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

// Normal Distribution
float D_GGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
	float denom2 = denom * denom;

	return a2 / (PI * denom2);
}

// Geometry Function
float G_Smith(float NdotL, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	float GL = NdotL / (NdotL * (1.0 - k) + k);
	float GV = NdotV / (NdotV * (1.0 - k) + k);

	return GL * GV;
}

// Fresnel Equation
float3 F_Schlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Cook-Torrance BRDF
float3 BRDF_CT(float3 L, float3 V, float3 N, float3 albedo, float metallic, float roughness)
{
	float3 H = normalize(V + L);
	float  NdotH = max(dot(N, H), 0.0);
	float  NdotL = max(dot(N, L), 0.0);
	float  NdotV = max(dot(N, V), 0.0);
	float  VdotH = max(dot(V, H), 0.0);

	if (NdotL <= 0.0 || NdotV <= 0.0)
		return float3(0.0);

	float3 F0 = lerp(float3(0.04), albedo, metallic);
	float  D = D_GGX(NdotH, roughness);
	float  G = G_Smith(NdotL, NdotV, roughness);
	float3 F = F_Schlick(VdotH, F0);

	float3 numerator = D * G * F;
	float  denominator = 4.0 * NdotL * NdotV + 0.0001;
	float3 specular = numerator / denominator;

	float3 kS = F;
	float3 kD = (1.0 - kS) * (1.0 - metallic);
	float3 diffuse = kD * albedo / PI;

	return diffuse + specular;
}

// Oren-Nayar BRDF
float BRDF_ON(float3 L, float3 V, float3 N, float roughness)
{
	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);

	if (NdotL <= 0.0 || NdotV <= 0.0)
		return 0.0;

	float sigma2 = roughness * roughness;

	float theta_i = acos(NdotL);
	float theta_r = acos(NdotV);
	float alpha = max(theta_i, theta_r);
	float beta = min(theta_i, theta_r);

	float A = 1.0 - 0.5 * sigma2 / (sigma2 + 0.33);
	float B = 0.45 * sigma2 / (sigma2 + 0.09);
	float C = sin(alpha) * tan(beta);

	float3 L_proj = normalize(L - N * NdotL);
	float3 V_proj = normalize(V - N * NdotV);

	float cos_phi_diff = max(dot(L_proj, V_proj), 0.0);

	return NdotL * (A + B * cos_phi_diff * C);
}
