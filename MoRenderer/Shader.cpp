#include "Shader.h"
#include "MoRenderer.h"


Vec4f BlinnPhongShader::VertexShaderFunction(int index, ShaderContext& output)
{
	Vec4f vertex = uniform_buffer_->mvp_matrix * vs_input_[index].position_os.xyz1();
	// 将顶点位置从模型空间转换为世界坐标系
	const Vec3f position_ws = (uniform_buffer_->model_matrix * vs_input_[index].position_os.xyz1()).xyz();

	output.varying_vec3f[VARYING_POSITION_WS] = position_ws;
	output.varying_vec2f[VARYING_TEXCOORD] = vs_input_[index].texcoord;
	return vertex;
}

Vec4f BlinnPhongShader::PixelShaderFunction(ShaderContext& input)
{
	return  1.0f;
}
