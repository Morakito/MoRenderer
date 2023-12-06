#include "Model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

static Vec4f calculate_tangent(Attributes* attribute[3], int index)
{
	//calculate the difference in UV coordinate
	Vec2f uvs[3] = {
		attribute[0]->texcoord,
		attribute[1]->texcoord ,
		attribute[2]->texcoord
	};

	Vec3f position_os[3] = {
	attribute[0]->position_os,
	attribute[1]->position_os ,
	attribute[2]->position_os
	};

	Vec3f normal = attribute[0]->normal_os;

	float x1 = uvs[1][0] - uvs[0][0];
	float y1 = uvs[1][1] - uvs[0][1];
	float x2 = uvs[2][0] - uvs[0][0];
	float y2 = uvs[2][1] - uvs[0][1];
	float det = (x1 * y2 - x2 * y1);

	//calculate the difference in world pos
	Vec3f e1 = position_os[1] - position_os[0];
	Vec3f e2 = position_os[2] - position_os[0];

	//calculate tangent-axis and bitangent-axis
	Vec3f t = e1 * y2 + e2 * (-y1);
	t /= det;
	t = vector_normalize(t - vector_dot(t, normal) * normal);

	return  t.xyz1();
}

Model::Model(const std::string& file_path, const std::string& file_name, const std::string& texture_format)
{
	// 加载OBJ模型
	{
		tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		std::string model_name = file_path + "/" + file_name + ".obj";
		if (!LoadObj(&attributes, &shapes, &materials, &warn, &err, model_name.c_str())) {
			throw std::runtime_error(warn + err);
		}

		vertex_number_ = 0;
		face_number_ = 0;

		for (const auto& shape : shapes) {
			for (size_t face_id = 0; face_id < shape.mesh.indices.size();) {

				// 构建一个顶点
				for (size_t i = 0; i < 3; i++) {
					Attributes attribute{};
					auto& index = shape.mesh.indices[face_id + i];
					attribute.position_os = {
						attributes.vertices[3 * index.vertex_index + 0],
						attributes.vertices[3 * index.vertex_index + 1],
						attributes.vertices[3 * index.vertex_index + 2]
					};

					// 部分uv值大于1，先将uv值转换到[0-1]区间中
					// uv坐标的原点位于左下角，贴图数据的原点位于左上角，因此需要在v轴上反向
					float u = attributes.texcoords[2 * index.texcoord_index + 0];
					float v = attributes.texcoords[2 * index.texcoord_index + 1];
					v = 1.0f - fmod(v, 1.0f);
					attribute.texcoord = { u,v };

					attribute.normal_os = {
						attributes.normals[3 * index.normal_index + 0],
						attributes.normals[3 * index.normal_index + 1],
						attributes.normals[3 * index.normal_index + 2],
					};
					attribute.tangent_os = Vec4f(1.0f, 0.0f, 0.0f, 1.0f);
					attributes_.push_back(attribute);
				}

				face_id += 3;
				vertex_number_ += 3;
				face_number_ += 1;
			}
		}
	}

	// 加载纹理
	{
		base_color_map_ = new Texture(GetTextureFileName(file_path, file_name, kTextureTypeBaseColor, texture_format));
		normal_map_ = new Texture(GetTextureFileName(file_path, file_name, kTextureTypeNormal, texture_format));
		roughness_map_ = new Texture(GetTextureFileName(file_path, file_name, kTextureTypeRoughness, texture_format));
		metallic_map_ = new Texture(GetTextureFileName(file_path, file_name, kTextureTypeMetallic, texture_format));
		occlusion_map_ = new Texture(GetTextureFileName(file_path, file_name, kTextureTypeOcclusion, texture_format));
		emission_map_ = new Texture(GetTextureFileName(file_path, file_name, kTextureTypeEmission, texture_format));
	}
}

Model::Model(std::vector<Vec3f>& vertex, const std::vector<int>& index)
{
	for (int i : index)
	{
		Attributes attribute{};
		attribute.position_os = {
			vertex[i]
		};
		attributes_.push_back(attribute);
	}
}



Model::~Model()
{
	delete base_color_map_;
	delete normal_map_;
	delete roughness_map_;
	delete metallic_map_;
	delete occlusion_map_;
	delete emission_map_;


	attributes_.clear();
}

std::string Model::GetTextureType(const TextureType texture_type)
{

	switch (texture_type)
	{
	case kTextureTypeBaseColor:			return "basecolor";
	case kTextureTypeNormal:			return "normal";
	case kTextureTypeRoughness:			return "roughness";
	case kTextureTypeMetallic:			return "metallic";
	case kTextureTypeOcclusion:			return "occlusion";
	case kTextureTypeEmission:			return "emission";

	default:							return "unknown";
	}
}

std::string Model::GetTextureFileName(const std::string& file_path, const std::string& file_name, const TextureType texture_type, const std::string& texture_format)
{
	const std::string texture_name = file_path + "/" + file_name + "_" + GetTextureType(texture_type) + texture_format;

	return texture_name;
}
