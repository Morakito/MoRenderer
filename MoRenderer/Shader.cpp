#include "Shader.h"
#include "MoRenderer.h"

#pragma region Blinn-Phong

Vec4f BlinnPhongShader::VertexShaderFunction(int index, Varings& output) const
{
	Vec4f position_cs = uniform_buffer_->mvp_matrix * attributes_[index].position_os.xyz1();
	const Vec3f position_ws = (uniform_buffer_->model_matrix * attributes_[index].position_os.xyz1()).xyz();
	const Vec3f normal_ws = (uniform_buffer_->normal_matrix * attributes_[index].normal_os.xyz1()).xyz();
	const Vec4f tangent_ws = uniform_buffer_->model_matrix * attributes_[index].tangent_os;

	output.varying_vec2f[VARYING_TEXCOORD] = attributes_[index].texcoord;
	output.varying_vec3f[VARYING_POSITION_WS] = position_ws;
	output.varying_vec3f[VARYING_NORMAL_WS] = normal_ws;
	output.varying_vec4f[VARYING_TANGENT_WS] = tangent_ws;
	return position_cs;
}

Vec4f BlinnPhongShader::PixelShaderFunction(Varings& input) const
{
	// ׼������
	Vec2f uv = input.varying_vec2f[VARYING_TEXCOORD];

	Vec3f normal_ws = input.varying_vec3f[VARYING_NORMAL_WS];
	if (model_->normal_map_->has_data_)
	{
		Vec4f tangent_ws = input.varying_vec4f[VARYING_TANGENT_WS];
		Vec3f perturb_normal = (model_->normal_map_->Sample2D(uv)).xyz();
		perturb_normal = perturb_normal * 2.0f - Vec3f(1.0f);
		normal_ws = calculate_normal(normal_ws, tangent_ws, perturb_normal);
	}
	normal_ws = vector_normalize(normal_ws);

	Vec3f position_ws = input.varying_vec3f[VARYING_POSITION_WS];

	Vec3f light_color = uniform_buffer_->light_color;
	Vec3f light_dir = vector_normalize(-uniform_buffer_->light_direction);
	Vec3f view_dir = vector_normalize(uniform_buffer_->camera_position - position_ws);

	// ������
	Vec3f ambient_color = Vec3f(0.1f);

	// ������
	Vec3f base_color = model_->base_color_map_->Sample2D(uv).xyz();
	Vec3f diffuse = light_color * base_color * Saturate(vector_dot(light_dir, normal_ws));

	// �߹�
	Vec3f half_dir = vector_normalize(view_dir + light_dir);
	float specular_intensity = pow(Saturate(vector_dot(normal_ws, half_dir)), 64);
	Vec3f specular = light_color * specular_intensity;

	Vec3f shaded_color = ambient_color + diffuse + specular;

	Vec3f display_color;
	switch (material_inspector_)
	{
	case kMaterialInspectorShaded:			display_color = shaded_color;	break;
	case kMaterialInspectorBaseColor:		display_color = base_color;		break;
	case kMaterialInspectorNormal:			display_color = normal_ws;		break;
	case kMaterialInspectorWorldPosition:	display_color = position_ws;	break;
	case kMaterialInspectorAmbient:			display_color = ambient_color;	break;
	case kMaterialInspectorDiffuse:			display_color = diffuse;		break;
	case kMaterialInspectorSpecular:		display_color = specular;		break;
	default:								display_color = shaded_color;
	}
	return display_color.xyz1();
}

void BlinnPhongShader::HandleKeyEvents()
{
	for (MaterialInspector i = kMaterialInspectorShaded;
		i <= kMaterialInspectorSpecular;
		i = static_cast<MaterialInspector>(i + 1))
	{
		if (window_->keys_[i])
		{
			material_inspector_ = i;
			if (i == kMaterialInspectorShaded)
			{
				window_->RemoveLogMessage("Material Inspector");
			}
			else
			{
				window_->SetLogMessage("Material Inspector", "Material Inspector: " + material_inspector_name_[i - 49]);
			}
			return;
		}
	}
}

#pragma endregion

#pragma region PBR

// �����������ʵ�Schlick����ֵ�����RTR4 �½�9.5
// ��F0��F90����ɫ��֮����в�ֵ��ʹ��pow����������ϣ��ڽӽ�����Ƕ��¿�������
// ���ھ���΢����ļ��裬���half_dir����΢����ķ��ߣ�cos(light_dir, half_dir)=cos(view_dir, half_dir)
Vec3f FresnelSchlickApproximation(const Vec3f& m, const Vec3f& light_dir, const Vec3f& f0)
{
	const float m_dot_l = Saturate(vector_dot(m, light_dir));
	return f0 + (Vec3f(1.0f) - f0) * pow(1.0f - m_dot_l, 5.0f);
}

// GGX���߷ֲ����������RTR4�½�9.8�еķ���9.41
float NormalDistributionGGX(const Vec3f& m, const Vec3f& n, const float perceptual_roughness)
{
	const float n_dot_m = Saturate(vector_dot(n, m));
	const float n_dot_m_2 = n_dot_m * n_dot_m;

	const float roughness = perceptual_roughness * perceptual_roughness;
	const float roughness2 = roughness * roughness;

	const float factor = 1.0f + n_dot_m_2 * (roughness2 - 1.0f);
	const float D = n_dot_m * roughness2 / (kPi * factor * factor);
	return D;
}

// Smith G1���������RTR4�½�9.7�еķ���9.24
// ʹ��GGX�ֲ���lambda����������½�9.8�еķ���9.37�ͷ���9.42
// �����е�s������view_dir����light_dir
float Smith_G1_GGX(const Vec3f& m, const Vec3f& n, const Vec3f& s, const float perceptual_roughness)
{
	const float m_dot_s = Saturate(vector_dot(m, s));
	const float n_dot_s = Saturate(vector_dot(n, s));
	const float n_dot_s_2 = n_dot_s * n_dot_s;

	const float roughness = perceptual_roughness * perceptual_roughness;
	const float roughness2 = roughness * roughness;

	const float a2_reciprocal = roughness2 * (1 - n_dot_s_2) / (n_dot_s_2 + kEpsilon);
	const float lambda = (sqrtf(1.0f + a2_reciprocal) - 1.0f) * 0.5f;

	const float Smith_G1 = m_dot_s / (1.0f + lambda);
	return Smith_G1;
}

// �����ʽ��G2���������RTR4�½�9.7�еķ���9.27
float Smith_G2_GGX(const Vec3f& m, const Vec3f& n, const Vec3f& light_dir, const Vec3f& view_dir, const float perceptual_roughness)
{
	const float g1_shadowing = Smith_G1_GGX(m, n, light_dir, perceptual_roughness);
	const float g1_masking = Smith_G1_GGX(m, n, view_dir, perceptual_roughness);

	return  g1_shadowing * g1_masking;
}

Vec4f PBRShader::VertexShaderFunction(int index, Varings& output) const
{
	Vec4f position_cs = uniform_buffer_->mvp_matrix * attributes_[index].position_os.xyz1();
	const Vec3f position_ws = (uniform_buffer_->model_matrix * attributes_[index].position_os.xyz1()).xyz();
	const Vec3f normal_ws = (uniform_buffer_->normal_matrix * attributes_[index].normal_os.xyz1()).xyz();
	const Vec4f tangent_ws = uniform_buffer_->model_matrix * attributes_[index].tangent_os;

	output.varying_vec2f[VARYING_TEXCOORD] = attributes_[index].texcoord;
	output.varying_vec3f[VARYING_POSITION_WS] = position_ws;
	output.varying_vec3f[VARYING_NORMAL_WS] = normal_ws;
	output.varying_vec4f[VARYING_TANGENT_WS] = tangent_ws;
	return position_cs;
}

Vec4f PBRShader::PixelShaderFunction(Varings& input) const
{
	Vec2f uv = input.varying_vec2f[VARYING_TEXCOORD];					// ��������
	Vec3f position_ws = input.varying_vec3f[VARYING_POSITION_WS];		// ����ռ�����

	Vec3f normal_ws = input.varying_vec3f[VARYING_NORMAL_WS];			// ����
	if (model_->normal_map_->has_data_)
	{
		Vec4f tangent_ws = input.varying_vec4f[VARYING_TANGENT_WS];
		Vec3f perturb_normal = (model_->normal_map_->Sample2D(uv)).xyz();
		perturb_normal = perturb_normal * 2.0f - Vec3f(1.0f);
		normal_ws = calculate_normal(normal_ws, tangent_ws, perturb_normal);
	}
	normal_ws = vector_normalize(normal_ws);

	float metallic = model_->metallic_map_->Sample2D(uv).b;				// ������
	float roughness = model_->roughness_map_->Sample2D(uv).b;				// �ֲڶ�
	float occlusion = model_->occlusion_map_->Sample2D(uv).b;				// �������ڱ�
	Vec3f emission = model_->emission_map_->Sample2D(uv).xyz();			// �Է���
	Vec3f base_color = model_->base_color_map_->Sample2D(uv).xyz();		// �ǽ�������Ϊalbedo����������ΪF0

	// ���������
	Vec3f light_color = uniform_buffer_->light_color;
	Vec3f light_dir = vector_normalize(-uniform_buffer_->light_direction);

	Vec3f view_dir = vector_normalize(uniform_buffer_->camera_position - position_ws);
	Vec3f half_dir = vector_normalize(view_dir + light_dir);

	float n_dot_l = vector_dot(normal_ws, light_dir);
	float n_dot_l_abs = Abs(n_dot_l);
	n_dot_l = Saturate(n_dot_l);
	float n_dot_v_abs = Abs(vector_dot(normal_ws, view_dir));

	// ���淴���BRDF
	Vec3f f0 = vector_lerp(dielectric_f0_, base_color, metallic);					// ��ȡ���ʵ�F0ֵ
	Vec3f F = FresnelSchlickApproximation(half_dir, light_dir, f0);					// ��������
	float D = NormalDistributionGGX(half_dir, normal_ws, roughness);				// ���߷ֲ���
	float G = Smith_G2_GGX(half_dir, normal_ws, light_dir, view_dir, roughness);	// shadowing masking��
	Vec3f cook_torrance_brdf = F * G * D / (4.0f * n_dot_l_abs * n_dot_v_abs + kEpsilon);

	// �α���ɢ��
	Vec3f lambertian_brdf = base_color;

	// ���㷴�䷽��
	Vec3f kd = (Vec3f(1.0f) - F) * (1 - metallic);
	Vec3f radiance_diffuse = kd * lambertian_brdf * light_color;
	Vec3f radiance_specular = cook_torrance_brdf * light_color * n_dot_l * 10.0f;

	Vec3f shaded_color = (radiance_diffuse + radiance_specular + emission) * occlusion;
	//shaded_color = radiance_diffuse;

	Vec3f display_color;
	switch (material_inspector_)
	{
	case kMaterialInspectorShaded:			display_color = shaded_color;	break;
	case kMaterialInspectorBaseColor:		display_color = base_color;		break;
	case kMaterialInspectorNormal:			display_color = normal_ws;		break;
	case kMaterialInspectorWorldPosition:	display_color = position_ws;	break;
	case kMaterialInspectorRoughness:		display_color = roughness;	break;
	case kMaterialInspectorMetallic:		display_color = metallic;		break;
	case kMaterialInspectorOcclusion:		display_color = occlusion;	break;
	case kMaterialInspectorEmission:		display_color = emission;		break;
		break;
	default:								display_color = shaded_color;
	}
	return display_color.xyz1();
}

void PBRShader::HandleKeyEvents()
{
	for (MaterialInspector i = kMaterialInspectorShaded;
		i <= kMaterialInspectorEmission;
		i = static_cast<MaterialInspector>(i + 1))
	{
		if (window_->keys_[i])
		{
			material_inspector_ = i;
			if (i == kMaterialInspectorShaded)
			{
				window_->RemoveLogMessage("Material Inspector");
			}
			else
			{
				window_->SetLogMessage("Material Inspector", "Material Inspector: " + material_inspector_name_[i - 49]);
			}
			return;
		}
	}
}
#pragma endregion

#pragma region SkyBox

Vec4f SkyBoxShader::VertexShaderFunction(int index, Varings& output) const
{
	Vec4f position_cs = uniform_buffer_->mvp_matrix * attributes_[index].position_os.xyz1();
	const Vec3f position_ws = (uniform_buffer_->model_matrix * attributes_[index].position_os.xyz1()).xyz();

	output.varying_vec3f[VARYING_POSITION_WS] = position_ws;
	return position_cs;
}

Vec4f SkyBoxShader::PixelShaderFunction(Varings& input) const
{
	Vec3f position_ws = input.varying_vec3f[VARYING_POSITION_WS];		// ����ռ�����
	return  cubemap_->Sample(position_ws).xyz1();
}

#pragma endregion 
