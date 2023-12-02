#include "Shader.h"
#include "MoRenderer.h"


Vec4f BlinnPhongShader::VertexShaderFunction(int index, ShaderContext& output) const
{
	Vec4f vertex = uniform_buffer_->mvp_matrix * attributes_[index].position_os.xyz1();
	// 将顶点位置从模型空间转换为世界坐标系
	const Vec3f position_ws = (uniform_buffer_->model_matrix * attributes_[index].position_os.xyz1()).xyz();

	output.varying_vec3f[VARYING_POSITION_WS] = position_ws;
	output.varying_vec2f[VARYING_TEXCOORD] = attributes_[index].texcoord;
	return vertex;
}

Vec4f BlinnPhongShader::PixelShaderFunction(ShaderContext& input)
{
	//Vec2f uv = input.varying_vec2f[BlinnPhongShader::VARYING_TEXCOORD];

	//Vec3f world_normal = (normal_map->Sample2D(uv) * uniform_buffer_->normal_matrix).xyz();
	//Vec3f world_position = input.varying_vec3f[BlinnPhongShader::VARYING_TEXCOORD];

	//Vec3f light_dir = vector_normalize(uniform_buffer_->light_direction);
	//Vec3f view_dir = vector_normalize(uniform_buffer_->camera_position - world_position);

	////漫反射
	//Vec4f base_color = diffuse_map->Sample2D(uv);
	//Vec4f diffuse = base_color * Saturate(vector_dot(light_dir, world_normal));

	////高光
	//float specular_scale = specular_map->Sample2D(uv).b * 5;
	//Vec3f half_dir = vector_normalize(view_dir + light_dir);
	//float intensity = pow(Saturate(vector_dot(world_normal, half_dir)), 20);
	//Vec4f specular = base_color * intensity * specular_scale;

	//Vec4f color = diffuse + specular;
	//return color;
	return 1.0f;
}
