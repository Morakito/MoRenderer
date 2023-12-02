#include "Shader.h"
#include "MoRenderer.h"


Vec4f BlinnPhongShader::VertexShaderFunction(int index, ShaderContext& output) const
{
	Vec4f vertex = uniform_buffer_->mvp_matrix * attributes_[index].position_os.xyz1();
	// ������λ�ô�ģ�Ϳռ�ת��Ϊ��������ϵ
	const Vec3f position_ws = (uniform_buffer_->model_matrix * attributes_[index].position_os.xyz1()).xyz();

	output.varying_vec3f[VARYING_POSITION_WS] = position_ws;
	output.varying_vec2f[VARYING_TEXCOORD] = attributes_[index].texcoord;
	return vertex;
}

Vec4f BlinnPhongShader::PixelShaderFunction(ShaderContext& input) const
{
	// ׼������
	Vec2f uv = input.varying_vec2f[VARYING_TEXCOORD];

	Vec3f world_normal = (uniform_buffer_->normal_matrix * model_->normal_map_->Sample2D(uv)).xyz();
	Vec3f world_position = input.varying_vec3f[VARYING_POSITION_WS];

	Vec4f light_color = uniform_buffer_->light_color;
	Vec3f light_dir = vector_normalize(-uniform_buffer_->light_direction);
	Vec3f view_dir = vector_normalize(uniform_buffer_->camera_position - world_position);

	// ������
	Vec4f ambient_color = { 0.1f,0.1f ,0.1f ,0.1f };

	// ������
	Vec4f base_color = model_->diffuse_map_->Sample2D(uv);
	Vec4f diffuse = light_color * base_color * Saturate(vector_dot(light_dir, world_normal));

	// �߹�
	Vec3f half_dir = vector_normalize(view_dir + light_dir);
	float specular_color = model_->specular_map_->Sample2D(uv).b * 5;
	float specular_intensity = pow(Saturate(vector_dot(world_normal, half_dir)), 32);
	Vec4f specular = light_color * base_color * specular_color * specular_intensity;

	Vec4f blinn_phong_color = ambient_color + diffuse + specular;
	return blinn_phong_color;
}
