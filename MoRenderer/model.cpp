#include "Model.h"

#include "utility.h"

Model::Model(const std::string& model_path, const Mat4x4f& model_matrix)
{
	// 加载OBJ模型
	LoadModel(model_path);

	model_folder_ = GetFileFolder(model_path);
	model_name_ = GetFileNameWithoutExtension(model_path);

	const std::string basecolor_file_name = GetFilePathByFileName(model_folder_, Model::GetTextureType(kTextureTypeBaseColor));
	std::string texture_format = GetFileExtension(basecolor_file_name);

	// 加载纹理
	{
		base_color_map_ = new Texture(GetTextureFileName(model_folder_, model_name_, kTextureTypeBaseColor, texture_format));
		normal_map_ = new Texture(GetTextureFileName(model_folder_, model_name_, kTextureTypeNormal, texture_format));
		roughness_map_ = new Texture(GetTextureFileName(model_folder_, model_name_, kTextureTypeRoughness, texture_format));
		metallic_map_ = new Texture(GetTextureFileName(model_folder_, model_name_, kTextureTypeMetallic, texture_format));
		occlusion_map_ = new Texture(GetTextureFileName(model_folder_, model_name_, kTextureTypeOcclusion, texture_format));
		emission_map_ = new Texture(GetTextureFileName(model_folder_, model_name_, kTextureTypeEmission, texture_format));
	}

	model_matrix_ = model_matrix;
}

void Model::LoadModel(const std::string& model_name)
{
	std::vector<Vec3f> positions;
	std::vector<Vec2f> texcoords;
	std::vector<Vec3f> normals;
	std::vector<Vec4f> tangents;

	std::vector<int> position_indices, texcoord_indices, normal_indices;

	constexpr int LINE_SIZE = 256;
	char line[LINE_SIZE];
	FILE* file = fopen(model_name.c_str(), "rb");
	while (true) {
		int items;
		if (fgets(line, LINE_SIZE, file) == NULL) {
			break;
		}
		else if (strncmp(line, "v ", 2) == 0) {               /* position */
			Vec3f position;
			items = sscanf(line, "v %f %f %f",
				&position.x, &position.y, &position.z);
			positions.push_back(position);
		}
		else if (strncmp(line, "vt ", 3) == 0) {              /* texcoord */
			Vec2f texcoord;
			items = sscanf(line, "vt %f %f",
				&texcoord.x, &texcoord.y);
			assert(items == 2);
			texcoords.push_back(texcoord);
		}
		else if (strncmp(line, "vn ", 3) == 0) {              /* normal */
			Vec3f normal;
			items = sscanf(line, "vn %f %f %f",
				&normal.x, &normal.y, &normal.z);
			assert(items == 3);
			normals.push_back(normal);
		}
		else if (strncmp(line, "f ", 2) == 0) {               /* face */
			int i;
			int pos_indices[3], uv_indices[3], n_indices[3];
			items = sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d",
				&pos_indices[0], &uv_indices[0], &n_indices[0],
				&pos_indices[1], &uv_indices[1], &n_indices[1],
				&pos_indices[2], &uv_indices[2], &n_indices[2]);
			assert(items == 9);
			for (i = 0; i < 3; i++) {
				position_indices.push_back(pos_indices[i] - 1);
				texcoord_indices.push_back(uv_indices[i] - 1);
				normal_indices.push_back(n_indices[i] - 1);
			}
		}
		else if (strncmp(line, "# ext.tangent ", 14) == 0) {  /* tangent */
			Vec4f tangent;
			items = sscanf(line, "# ext.tangent %f %f %f %f",
				&tangent.x, &tangent.y, &tangent.z, &tangent.w);
			assert(items == 4);
			tangents.push_back(tangent);
		}
	}


	face_number_ = position_indices.size() / 3;
	vertex_number_ = position_indices.size();

	for (int i = 0; i < position_indices.size(); i++) {
		int position_index = position_indices[i];
		int texcoord_index = texcoord_indices[i];
		int normal_index = normal_indices[i];

		Attributes attribute{};
		attribute.position_os = positions[position_index];
		attribute.normal_os = normals[normal_index];

		// 部分uv值大于1，先将uv值转换到[0-1]区间中
		// uv坐标的原点位于左下角，贴图数据的原点位于左上角，因此需要在v轴上反向
		float u = texcoords[texcoord_index].u;
		float v = texcoords[texcoord_index].v;
		v = 1.0f - fmod(v, 1.0f);
		attribute.texcoord = { u,v };

		if (tangents.size() > 0)
		{
			attribute.tangent_os = tangents[position_index];
		}
		else
		{
			attribute.tangent_os = Vec4f(1.0f, 0.0f, 0.0f, 1.0f);
		}

		attributes_.push_back(attribute);
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

std::string Model::PrintModelInfo()
{
	const std::string model_message =
		"vertex count: " + std::to_string(vertex_number_) +
		"  face count: " + std::to_string(face_number_) + "\n";

	return model_message;
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
