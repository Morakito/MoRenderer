#ifndef SHADER_H
#define SHADER_H

#include "math.h"
#include  <map>
#include  <functional>

#include "model.h"
#include "Window.h"


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
	Vec3f light_color;			// ������ɫ
	Vec3f camera_position;		// �������

};

// ��ɫ�������ģ��� VS ���ã�������Ⱦ������������ֵ�󣬹� PS ��ȡ
struct Varings {
	std::map<int, float> varying_float;    // ������ varying �б�
	std::map<int, Vec2f> varying_vec2f;    // ��άʸ�� varying �б�
	std::map<int, Vec3f> varying_vec3f;    // ��άʸ�� varying �б�
	std::map<int, Vec4f> varying_vec4f;    // ��άʸ�� varying �б�
};

enum ShaderType
{
	kBlinnPhongShader,
	kPbrShader,
	kSkyBoxShader
};



// ������ɫ�������ض���Ĳü��ռ�����
typedef std::function<Vec4f(int index, Varings& output)> VertexShader;

// ������ɫ�����������ص���ɫ
typedef std::function<Vec4f(Varings& input)> PixelShader;

class IShader
{

public:
	virtual ~IShader() = default;

	IShader()
	{
		uniform_buffer_ = new UniformBuffer();
		attributes_ = new Attributes[3];
		window_ = Window::GetInstance();

		vertex_shader_ = [&](const int index, Varings& output)->Vec4f
			{
				return VertexShaderFunction(index, output);
			};
		pixel_shader_ = [&](Varings& input)->Vec4f
			{
				return PixelShaderFunction(input);
			};
	}

	virtual  Vec4f VertexShaderFunction(int index, Varings& output) const = 0;
	virtual  Vec4f PixelShaderFunction(Varings& input) const = 0;
	virtual void HandleKeyEvents() = 0;

public:
	UniformBuffer* uniform_buffer_;
	Attributes* attributes_;
	Model* model_;

	VertexShader vertex_shader_;
	PixelShader pixel_shader_;

	Window* window_;				// ���ڻ�ȡ�����¼���չʾ�������
};

// Blinn Phong����ģ��
class BlinnPhongShader final :public IShader
{
public:
	BlinnPhongShader()
	{

	}
	Vec4f VertexShaderFunction(int index, Varings& output) const override;
	Vec4f PixelShaderFunction(Varings& input) const override;
	void HandleKeyEvents()  override;

public:
	enum VaryingAttributes
	{
		VARYING_TEXCOORD = 0,			// ��������
		VARYING_POSITION_WS = 1,		// ����ռ�����
		VARYING_NORMAL_WS = 2,			// ����ռ䷨��
		VARYING_TANGENT_WS = 3			// ����ռ�����
	};

	enum MaterialInspector
	{
		kMaterialInspectorShaded = '1',
		kMaterialInspectorBaseColor,
		kMaterialInspectorNormal,
		kMaterialInspectorWorldPosition,
		kMaterialInspectorAmbient,
		kMaterialInspectorDiffuse,
		kMaterialInspectorSpecular
	};

	const std::string material_inspector_name_[7] =
	{
		"Shaded",
		"BaseColor",
		"Normal",
		"WorldPosition",
		"Ambient",
		"Diffuse",
		"Specular"
	};

	MaterialInspector material_inspector_;
};

// PBR����ģ�ͣ�ʹ��metallic������
class PBRShader final :public IShader
{
public:
	PBRShader()
	{
		// �ǽ�����F0ֵĬ��Ϊ0.04
		dielectric_f0_ = Vec3f(0.04f);
		material_inspector_ = kMaterialInspectorShaded;
	}
	Vec4f VertexShaderFunction(int index, Varings& output) const override;
	Vec4f PixelShaderFunction(Varings& input) const override;
	void HandleKeyEvents() override;

	enum VaryingAttributes
	{
		VARYING_TEXCOORD = 0,			// ��������
		VARYING_POSITION_WS = 1,		// ����ռ�����
		VARYING_NORMAL_WS = 2,			// ����ռ䷨��
		VARYING_TANGENT_WS = 3			// ����ռ�����
	};

	enum MaterialInspector
	{
		kMaterialInspectorShaded = '1',
		kMaterialInspectorBaseColor,
		kMaterialInspectorNormal,
		kMaterialInspectorWorldPosition,
		kMaterialInspectorRoughness,
		kMaterialInspectorMetallic,
		kMaterialInspectorOcclusion,
		kMaterialInspectorEmission
	};

	const std::string material_inspector_name_[8] =
	{
		"Shaded",
		"BaseColor",
		"Normal",
		"WorldPosition",
		"Roughness",
		"Metallic",
		"Occlusion",
		"Emission"
	};

	MaterialInspector material_inspector_;
	Vec3f dielectric_f0_;
};


class SkyBoxShader final :public IShader
{
public:
	SkyBoxShader()
	{
		
	}

	Vec4f VertexShaderFunction(int index, Varings& output) const override;
	Vec4f PixelShaderFunction(Varings& input) const override;
	void HandleKeyEvents() override {};

	enum VaryingAttributes
	{
		VARYING_POSITION_WS = 0,		// ����ռ�����
	};

public:
	CubeMap* cubemap_;

	std::vector<Vec3f> plane_vertex_ = {
		{0.5f,0.5f,0.5f},			// ���Ͻ�
		{-0.5f,0.5f,0.5f},			// ���Ͻ�
		{-0.5f,-0.5f,0.5f},		// ���½�
		{0.5f,-0.5f,0.5f} };		// ���½�
	std::vector<int> plane_index_ = { 0,1,2,     0,2,3 };
};

#endif // !SHADER_H

