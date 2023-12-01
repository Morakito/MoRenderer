#include "model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Model::Model(const std::string file_name)
{
	{
		tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warn, &err, file_name.c_str())) {
			throw std::runtime_error(warn + err);
		}

		vertex_number_ = 0;
		face_number_ = 0;

		for (const auto& shape : shapes) {
			for (size_t face_id = 0; face_id < shape.mesh.indices.size();) {

				//构建一个顶点
				for (int i = 0; i < 3; i++) {
					Attributes vertex{};
					auto& index = shape.mesh.indices[face_id + i];
					vertex.position_os = {
						attributes.vertices[3 * index.vertex_index + 0],
						attributes.vertices[3 * index.vertex_index + 1],
						attributes.vertices[3 * index.vertex_index + 2]
					};
					vertex.texcoord = {
						attributes.texcoords[2 * index.texcoord_index + 0],
						1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
					};
					vertex.normal = {
						attributes.normals[3 * index.normal_index + 0],
						attributes.normals[3 * index.normal_index + 1],
						attributes.normals[3 * index.normal_index + 2],
					};
					vertices_.push_back(vertex);
				}
				face_id += 3;
				vertex_number_ += 3;
				face_number_ += 1;
			}
		}
	}

}
