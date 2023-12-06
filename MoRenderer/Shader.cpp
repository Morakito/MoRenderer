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
	// 准备数据
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

	// 环境光
	Vec3f ambient_color = Vec3f(0.1f);

	// 漫反射
	Vec3f base_color = model_->base_color_map_->Sample2D(uv).xyz();
	Vec3f diffuse = light_color * base_color * Saturate(vector_dot(light_dir, normal_ws));

	// 高光
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

// 菲涅尔反射率的Schlick近似值，详见RTR4 章节9.5
// 在F0和F90（白色）之间进行插值，使用pow函数进行拟合，在接近掠射角度下快速增长
// 由于镜面微表面的假设，因此half_dir就是微表面的法线，cos(light_dir, half_dir)=cos(view_dir, half_dir)
Vec3f FresnelSchlickApproximation(const Vec3f& m, const Vec3f& light_dir, const Vec3f& f0)
{
	const float m_dot_l = Saturate(vector_dot(m, light_dir));
	return f0 + (Vec3f(1.0f) - f0) * pow(1.0f - m_dot_l, 5.0f);
}

// GGX法线分布函数，详见RTR4章节9.8中的方程9.41
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

// Smith G1函数，详见RTR4章节9.7中的方程9.24
// 使用GGX分布的lambda函数，详见章节9.8中的方程9.37和方程9.42
// 参数中的s可能是view_dir或者light_dir
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

// 最简单形式的G2函数，详见RTR4章节9.7中的方程9.27
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
	Vec2f uv = input.varying_vec2f[VARYING_TEXCOORD];					// 纹理坐标
	Vec3f position_ws = input.varying_vec3f[VARYING_POSITION_WS];		// 世界空间坐标

	Vec3f normal_ws = input.varying_vec3f[VARYING_NORMAL_WS];			// 法线
	if (model_->normal_map_->has_data_)
	{
		Vec4f tangent_ws = input.varying_vec4f[VARYING_TANGENT_WS];
		Vec3f perturb_normal = (model_->normal_map_->Sample2D(uv)).xyz();
		perturb_normal = perturb_normal * 2.0f - Vec3f(1.0f);
		normal_ws = calculate_normal(normal_ws, tangent_ws, perturb_normal);
	}
	normal_ws = vector_normalize(normal_ws);

	float metallic = model_->metallic_map_->Sample2D(uv).b;				// 金属度
	float roughness = model_->roughness_map_->Sample2D(uv).b;				// 粗糙度
	float occlusion = model_->occlusion_map_->Sample2D(uv).b;				// 环境光遮蔽
	Vec3f emission = model_->emission_map_->Sample2D(uv).xyz();			// 自发光
	Vec3f base_color = model_->base_color_map_->Sample2D(uv).xyz();		// 非金属部分为albedo，金属部分为F0

	// 方向光数据
	Vec3f light_color = uniform_buffer_->light_color;
	Vec3f light_dir = vector_normalize(-uniform_buffer_->light_direction);

	Vec3f view_dir = vector_normalize(uniform_buffer_->camera_position - position_ws);
	Vec3f half_dir = vector_normalize(view_dir + light_dir);

	float n_dot_l = vector_dot(normal_ws, light_dir);
	float n_dot_l_abs = Abs(n_dot_l);
	n_dot_l = Saturate(n_dot_l);
	float n_dot_v_abs = Abs(vector_dot(normal_ws, view_dir));

	// 表面反射的BRDF
	Vec3f f0 = vector_lerp(dielectric_f0_, base_color, metallic);					// 获取材质的F0值
	Vec3f F = FresnelSchlickApproximation(half_dir, light_dir, f0);					// 菲涅尔项
	float D = NormalDistributionGGX(half_dir, normal_ws, roughness);				// 法线分布项
	float G = Smith_G2_GGX(half_dir, normal_ws, light_dir, view_dir, roughness);	// shadowing masking项
	Vec3f cook_torrance_brdf = F * G * D / (4.0f * n_dot_l_abs * n_dot_v_abs + kEpsilon);

	// 次表面散射
	Vec3f lambertian_brdf = base_color;

	// 计算反射方程
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
	Vec3f position_ws = input.varying_vec3f[VARYING_POSITION_WS];		// 世界空间坐标
	return  cubemap_->Sample(position_ws).xyz1();
}

#pragma endregion 
