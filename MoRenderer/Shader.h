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

		// ���ڽ����ߴ�ģ�Ϳռ�任������ռ�
		// ʹ��ԭʼ�任�������ת�þ���
		normal_matrix = matrix_invert(model_matrix).Transpose();
	}
};

// ��ɫ�������ģ��� VS ���ã�������Ⱦ������������ֵ�󣬹� PS ��ȡ
struct ShaderContext {
	std::map<int, float> varying_float;    // ������ varying �б�
	std::map<int, Vec2f> varying_vec2f;    // ��άʸ�� varying �б�
	std::map<int, Vec3f> varying_vec3f;    // ��άʸ�� varying �б�
	std::map<int, Vec4f> varying_vec4f;    // ��άʸ�� varying �б�
};


// ������ɫ������Ϊ�� C++ ��д�����贫�� attribute������ 0-2 �Ķ������
// ��ɫ������ֱ������������Ŷ�ȡ��Ӧ���ݼ��ɣ������Ҫ����һ������ position_
// ���� varying ���õ� output �����Ⱦ����ֵ�󴫵ݸ� PS 
typedef std::function<Vec4f(int index, ShaderContext& output)> VertexShader;


// ������ɫ�������� ShaderContext����Ҫ���� Vec4f ���͵���ɫ
// ��������ÿ����� input ����ֵ�����ǰ����������� output ��ֵ�õ�
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
		VARYING_TEXCOORD = 0,			// ��������
		VARYING_POSITION_WS = 1			// ��������
	};

	UniformBuffer* uniform_buffer_;
	Attributes* vs_input_;

	VertexShader vertex_shader_;
};

#endif // !SHADER_H

