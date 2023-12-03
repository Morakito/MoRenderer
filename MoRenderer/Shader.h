#ifndef SHADER_H
#define SHADER_H

#include "math.h"
#include  <map>
#include  <functional>

#include "model.h"

struct UniformBuffer
{
	Mat4x4f model_matrix;		// ģ�ͱ任����
	Mat4x4f view_matrix;		// �۲�任����
	Mat4x4f proj_matrix;		// ͶӰ�任����
	Mat4x4f mvp_matrix;			// MVP�任����

	Mat4x4f normal_matrix;		// ���߱任����

	void CalculateRestMatrix() {
		mvp_matrix = proj_matrix * view_matrix * model_matrix;

		// ���ڽ����ߴ�ģ�Ϳռ�任������ռ�
		// ʹ��ԭʼ�任�������ת�þ���
		normal_matrix = matrix_invert(model_matrix).Transpose();
	}

	// ��������
	Vec3f light_direction;		// ���շ��򣬴���ɫ��ָ���Դ
	Vec4f light_color;			// ������ɫ
	Vec3f camera_position;		// �������

};

// ��ɫ�������ģ��� VS ���ã�������Ⱦ������������ֵ�󣬹� PS ��ȡ
struct ShaderContext {
	std::map<int, float> varying_float;    // ������ varying �б�
	std::map<int, Vec2f> varying_vec2f;    // ��άʸ�� varying �б�
	std::map<int, Vec3f> varying_vec3f;    // ��άʸ�� varying �б�
	std::map<int, Vec4f> varying_vec4f;    // ��άʸ�� varying �б�
};


// ������ɫ�������ض���Ĳü��ռ�����
typedef std::function<Vec4f(int index, ShaderContext& output)> VertexShader;


// ������ɫ�����������ص���ɫ
typedef std::function<Vec4f(ShaderContext& input)> PixelShader;

// Blinn Phong����ģ��
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
		VARYING_TEXCOORD = 0,			// ��������
		VARYING_POSITION_WS = 1,		// ����ռ�����
		VARYING_NORMAL_WS = 2,			// ����ռ䷨��
		VARYING_TANGENT_WS = 3			// ����ռ�����

	};

	UniformBuffer* uniform_buffer_;
	Attributes* attributes_;
	Model* model_;

	VertexShader vertex_shader_;
	PixelShader pixel_shader_;
};

#endif // !SHADER_H

