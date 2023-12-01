#ifndef MODEL_H
#define MODEL_H

#include "math.h"
#include <vector>


struct Attributes {
	Vec3f position_os;
	Vec2f texcoord;
	Vec3f normal;
};

class Model {
public:
	explicit Model(const std::string file_name);

public:
	std::vector<Attributes> vertices_;

	int vertex_number_;
	int face_number_;
};


#endif


