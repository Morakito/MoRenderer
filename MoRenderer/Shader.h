#ifndef SHADER_H
#define SHADER_H

#include "math.h"
#include  <map>
#include  <functional>

#include "model.h"

struct UniformBuffer
{
	Mat4x4f model_matrix;
	Mat4x4f view_matrix;
	Mat4x4f proj_matrix;
	Mat4x4f mvp_matrix;

	Mat4x4f normal_matrix;

	void CalculateRestMatrix() {
		mvp_matrix = proj_matrix * view_matrix * model_matrix;

		// 用于将法线从模型空间变换到世界空间
		// 使用原始变换矩阵的逆转置矩阵
		normal_matrix = matrix_invert(model_matrix).Transpose();
	}
};

// 着色器上下文，由 VS 设置，再由渲染器按像素逐点插值后，供 PS 读取
struct ShaderContext {
	std::map<int, float> varying_float;    // 浮点数 varying 列表
	std::map<int, Vec2f> varying_vec2f;    // 二维矢量 varying 列表
	std::map<int, Vec3f> varying_vec3f;    // 三维矢量 varying 列表
	std::map<int, Vec4f> varying_vec4f;    // 四维矢量 varying 列表
};


// 顶点着色器：因为是 C++ 编写，无需传递 attribute，传个 0-2 的顶点序号
// 着色器函数直接在外层根据序号读取相应数据即可，最后需要返回一个坐标 position_
// 各项 varying 设置到 output 里，由渲染器插值后传递给 PS 
typedef std::function<Vec4f(int index, ShaderContext& output)> VertexShader;


// 像素着色器：输入 ShaderContext，需要返回 Vec4f 类型的颜色
// 三角形内每个点的 input 具体值会根据前面三个顶点的 output 插值得到
typedef std::function<Vec4f(ShaderContext& input)> PixelShader;

class BlinnPhongShader
{
public:
	BlinnPhongShader()
	{
		uniform_buffer_ = new UniformBuffer();
		vs_input_ = new Attributes[3];

		vertex_shader_ = [&](const int index, ShaderContext& output)->Vec4f
			{
				return VertexShaderFunction(index, output);
			};
	}

	Vec4f VertexShaderFunction(int index, ShaderContext& output) ;
	Vec4f PixelShaderFunction(ShaderContext& input) ;

public:
	enum VaryingAttributes
	{
		VARYING_TEXCOORD = 0,			// 纹理坐标
		VARYING_POSITION_WS = 1			// 世界坐标
	};

	UniformBuffer* uniform_buffer_;
	Attributes* vs_input_;

	VertexShader vertex_shader_;
};

#endif // !SHADER_H

