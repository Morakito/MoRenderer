#ifndef SHADER_H
#define SHADER_H

#include "math.h"

struct UniformBuffer
{
	Mat4x4f modelMatrix;
	Mat4x4f viewMatrix;
	Mat4x4f projMatrix;
	Mat4x4f mvpMatrix;

	Mat4x4f normalMatrix;

	void CalculateRestMatrix() {
		mvpMatrix = modelMatrix * viewMatrix * projMatrix;

		// 用于将法线从模型空间变换到世界空间
		// 使用原始变换矩阵的逆转置矩阵
		normalMatrix = matrix_invert(modelMatrix).Transpose();
	}
};

#endif // !SHADER_H

