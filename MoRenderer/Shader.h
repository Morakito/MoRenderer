#ifndef SHADER_H
#define SHADER_H

#include "math.h"

struct UniformBuffer
{
	Mat4x4f model_matrix;
	Mat4x4f view_matrix;
	Mat4x4f proj_matrix;
	Mat4x4f mvp_matrix;

	Mat4x4f normal_matrix;

	void CalculateRestMatrix() {
		mvp_matrix = proj_matrix* view_matrix * model_matrix;

		// 用于将法线从模型空间变换到世界空间
		// 使用原始变换矩阵的逆转置矩阵
		normal_matrix = matrix_invert(model_matrix).Transpose();
	}
};

#endif // !SHADER_H

