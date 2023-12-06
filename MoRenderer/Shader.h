#ifndef SHADER_H
#define SHADER_H

#include "math.h"
#include  <map>
#include  <functional>

#include "model.h"
#include "Window.h"


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
	Vec3f light_color;			// 光照颜色
	Vec3f camera_position;		// 相机方向

};

// 着色器上下文，由 VS 设置，再由渲染器按像素逐点插值后，供 PS 读取
struct Varings {
	std::map<int, float> varying_float;    // 浮点数 varying 列表
	std::map<int, Vec2f> varying_vec2f;    // 二维矢量 varying 列表
	std::map<int, Vec3f> varying_vec3f;    // 三维矢量 varying 列表
	std::map<int, Vec4f> varying_vec4f;    // 四维矢量 varying 列表
};

enum ShaderType
{
	kBlinnPhongShader,
	kPbrShader,
	kSkyBoxShader
};



// 顶点着色器：返回顶点的裁剪空间坐标
typedef std::function<Vec4f(int index, Varings& output)> VertexShader;

// 像素着色器：返回像素的颜色
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

	Window* window_;				// 用于获取按键事件，展示材质面板
};

// Blinn Phong光照模型
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
		VARYING_TEXCOORD = 0,			// 纹理坐标
		VARYING_POSITION_WS = 1,		// 世界空间坐标
		VARYING_NORMAL_WS = 2,			// 世界空间法线
		VARYING_TANGENT_WS = 3			// 世界空间切线
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

// PBR光照模型，使用metallic工作流
class PBRShader final :public IShader
{
public:
	PBRShader()
	{
		// 非金属的F0值默认为0.04
		dielectric_f0_ = Vec3f(0.04f);
		material_inspector_ = kMaterialInspectorShaded;
	}
	Vec4f VertexShaderFunction(int index, Varings& output) const override;
	Vec4f PixelShaderFunction(Varings& input) const override;
	void HandleKeyEvents() override;

	enum VaryingAttributes
	{
		VARYING_TEXCOORD = 0,			// 纹理坐标
		VARYING_POSITION_WS = 1,		// 世界空间坐标
		VARYING_NORMAL_WS = 2,			// 世界空间法线
		VARYING_TANGENT_WS = 3			// 世界空间切线
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
		VARYING_POSITION_WS = 0,		// 世界空间坐标
	};

public:
	CubeMap* cubemap_;

	std::vector<Vec3f> plane_vertex_ = {
		{0.5f,0.5f,0.5f},			// 右上角
		{-0.5f,0.5f,0.5f},			// 左上角
		{-0.5f,-0.5f,0.5f},		// 左下角
		{0.5f,-0.5f,0.5f} };		// 右下角
	std::vector<int> plane_index_ = { 0,1,2,     0,2,3 };
};

#endif // !SHADER_H

