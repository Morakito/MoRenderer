#include "model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Model::Model(const std::string fileName)
{
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fileName.c_str())) {
			throw std::runtime_error(warn + err);
		}

		for (const auto& shape : shapes) {
			for (size_t face_id = 0; face_id < shape.mesh.indices.size();) {

				//构建一个顶点
				for (int i = 0; i < 3; i++) {
					Vertex vertex{};
					auto& index = shape.mesh.indices[face_id + i];
					vertex.positionOS = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};
					vertex.texcoord = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
					};
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2],
					};
					vertices.push_back(vertex);
				}
				face_id += 3;
			}
		}
	}

}
