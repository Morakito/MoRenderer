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

		// ���ڽ����ߴ�ģ�Ϳռ�任������ռ�
		// ʹ��ԭʼ�任�������ת�þ���
		normalMatrix = matrix_invert(modelMatrix).Transpose();
	}
};

#endif // !SHADER_H

