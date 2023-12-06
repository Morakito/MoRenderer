#ifndef MODEL_H
#define MODEL_H

#include <vector>

#include "math.h"
#include "Texture.h"

struct Attributes {
	Vec3f position_os;
	Vec2f texcoord;
	Vec3f normal_os;
	Vec4f tangent_os;
};

enum TextureType
{
	kTextureTypeBaseColor,
	kTextureTypeNormal,
	kTextureTypeRoughness,
	kTextureTypeMetallic,
	kTextureTypeOcclusion,
	kTextureTypeEmission
};

class Model {
public:

	// 用于加载外部模型
	explicit Model(const std::string& file_path, const std::string& file_name, const std::string& texture_format);

	// 用于程序化生成模型
	Model(std::vector<Vec3f>& vertex, const std::vector<int>& index);

	~Model();

public:
	static std::string GetTextureType(TextureType texture_type);
	static std::string GetTextureFileName(const std::string& file_path, const std::string& file_name, TextureType texture_type, const std::string& texture_format);

public:
	std::vector<Attributes> attributes_;

	int vertex_number_;
	int face_number_;

	Texture* base_color_map_;
	Texture* normal_map_;
	Texture* roughness_map_;
	Texture* metallic_map_;
	Texture* occlusion_map_;
	Texture* emission_map_;
};

#endif


