#ifndef MODEL_H
#define MODEL_H

#include "math.h"
#include "Texture.h"
#include <vector>


struct Attributes {
	Vec3f position_os;
	Vec2f texcoord;
	Vec3f normal;
};

enum TextureType
{
	DIFFUSE,
	NORMAL,
	SPECULAR
};

class Model {
public:
	explicit Model(const std::string file_path, const std::string file_name, const std::string texture_format);

	~Model();


public:
	static std::string GetTextureType(TextureType texture_type);
	static std::string GetTextureFileName(const std::string& file_path, const std::string& file_name, TextureType texture_type, const std::string& texture_format);

public:
	std::vector<Attributes> vertices_;

	int vertex_number_;
	int face_number_;

	Texture* diffuse_map_;
	Texture* normal_map_;
	Texture* specular_map_;
};


#endif


