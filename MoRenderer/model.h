#ifndef MODEL_H
#define MODEL_H

#include "math.h"
#include <vector>


struct Vertex {
	Vec3f positionOS;
	Vec2f texcoord;
	Vec3f normal;
};

class Model {
public:

	Model(const std::string fileName);

public:
	std::vector<Vertex> vertices;

};


#endif


