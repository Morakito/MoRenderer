#ifndef CAMERA_H
#define CAMERA_H

#include "math.h"

#include "Shader.h"

class Camera
{
public:
	Camera(Vec3f _position, Vec3f _target, Vec3f _up, float _fov, float _aspect);
	~Camera();

	// ���������¼�
	void HandleInputEvents();

	// ����uniformbuffer�еľ���
	void UpdateUniformBuffer(UniformBuffer* uniformbuffer);

private:
	// ���������̬
	void UpdataCameraPose();
	void HandleMouseEvents();
	void HandleKeyEvents();


public:
	Vec3f position;					// ���������ռ�λ��
	Vec3f target;					// ������������ռ�λ��
	Vec3f up;						// �����up����

	/*
		�������ϵ����
		axisX��������ָ����Ļ�Ҳ�
		axisY��������ָ����Ļ�ϲ�
		axisZ��������ָ����Ļ
	*/
	Vec3f axisX, axisY, axisZ;		

	float fov;						// ���FOV
	float aspect;					// �����

	float nearPlane;				// ���ü�ƽ��
	float farPlane;					// Զ�ü�ƽ��
};

#endif // !CAMERA_H
