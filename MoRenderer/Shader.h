#ifndef SHADER_H
#define SHADER_H

#include "math.h"
#include  <map>
#include  <functional>

#include "model.h"

struct UniformBuffer
{
	Mat4x4f model_matrix;		// 模型变换矩阵
	Mat4x4f view_matrix;		// 观察变换矩阵
	Mat4x4f proj_matrix;		// 投影变换矩阵
	Mat4x4f mvp_matrix;			// MVP变换矩阵

	Mat4x4f normal_matrix;		// 法线变换矩阵

	void CalculateRestMatrix() {
		mvp_matrix = proj_matrix * view_matrix * model_matrix;

		// 用于将法线从模型空间变换到世界空间
		// 使用原始变换矩阵的逆转置矩阵
		normal_matrix = matrix_invert(model_matrix).Transpose();
	}

	// 光照数据
	Vec3f light_direction;		// 光照方向，从着色点指向光源
	Vec4f light_color;			// 光照颜色
	Vec3f camera_position;		// 相机方向

};

// 着色器上下文，由 VS 设置，再由渲染器按像素逐点插值后，供 PS 读取
struct ShaderContext {
	std::map<int, float> varying_float;    // 浮点数 varying 列表
	std::map<int, Vec2f> varying_vec2f;    // 二维矢量 varying 列表
	std::map<int, Vec3f> varying_vec3f;    // 三维矢量 varying 列表
	std::map<int, Vec4f> varying_vec4f;    // 四维矢量 varying 列表
};


// 顶点着色器：返回顶点的裁剪空间坐标
typedef std::function<Vec4f(int index, ShaderContext& output)> VertexShader;


// 像素着色器：返回像素的颜色
typedef std::function<Vec4f(ShaderContext& input)> PixelShader;

// Blinn Phong光照模型
class BlinnPhongShader
{
public:
	BlinnPhongShader()
	{
		uniform_buffer_ = new UniformBuffer();
		attributes_ = new Attributes[3];

		vertex_shader_ = [&](const int index, ShaderContext& output)->Vec4f
			{
				return VertexShaderFunction(index, output);
			};
		pixel_shader_ = [&](ShaderContext& input)->Vec4f
			{
				return PixelShaderFunction(input);
			};
	}

	Vec4f VertexShaderFunction(int index, ShaderContext& output) const;
	Vec4f PixelShaderFunction(ShaderContext& input) const;

public:
	enum VaryingAttributes
	{
		VARYING_TEXCOORD = 0,			// 纹理坐标
		VARYING_POSITION_WS = 1,		// 世界空间坐标
		VARYING_NORMAL_WS = 2,			// 世界空间法线
		VARYING_TANGENT_WS = 3			// 世界空间切线

	};

	UniformBuffer* uniform_buffer_;
	Attributes* attributes_;
	Model* model_;

	VertexShader vertex_shader_;
	PixelShader pixel_shader_;
};

#endif // !SHADER_H

