#include "Shader.h"
#include "MoRenderer.h"


Vec4f BlinnPhongShader::VertexShaderFunction(int index, ShaderContext& output) const
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

Vec4f BlinnPhongShader::PixelShaderFunction(ShaderContext& input) const
{
	// 准备数据
	Vec2f uv = input.varying_vec2f[VARYING_TEXCOORD];

	Vec3f normal_ws = input.varying_vec3f[VARYING_NORMAL_WS];
	Vec4f tangent_ws = input.varying_vec4f[VARYING_TANGENT_WS];
	Vec3f perturb_normal = (model_->normal_map_->Sample2D(uv)).xyz();
	perturb_normal = perturb_normal * 2.0f - Vec3f(1.0f);
	normal_ws = calculate_normal(normal_ws, tangent_ws, perturb_normal);

	Vec3f world_position = input.varying_vec3f[VARYING_POSITION_WS];

	Vec4f light_color = uniform_buffer_->light_color;
	Vec3f light_dir = vector_normalize(-uniform_buffer_->light_direction);
	Vec3f view_dir = vector_normalize(uniform_buffer_->camera_position - world_position);

	// 环境光
	Vec4f ambient_color = { 0.1f,0.1f ,0.1f ,0.1f };

	// 漫反射

	std::cout << uv << std::endl;
	Vec4f base_color = model_->base_color_map_->Sample2D(uv);
	//std::cout << base_color << std::endl;
	Vec4f diffuse = light_color * base_color * Saturate(vector_dot(light_dir, normal_ws));

	// 高光
	Vec3f half_dir = vector_normalize(view_dir + light_dir);
	float specular_intensity = pow(Saturate(vector_dot(normal_ws, half_dir)), 64);
	Vec4f specular = light_color * specular_intensity;

	Vec4f blinn_phong_color = ambient_color + diffuse + specular;
	blinn_phong_color = diffuse;
	return blinn_phong_color;
}
