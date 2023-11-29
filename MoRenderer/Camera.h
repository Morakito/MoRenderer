#ifndef CAMERA_H
#define CAMERA_H

#include "math.h"

#include "Shader.h"

class Camera
{
public:
	Camera(Vec3f position, Vec3f target, Vec3f up, float fov, float aspect);
	~Camera();

	// ���������¼�
	void HandleInputEvents();

	// ����uniform buffer�еľ���
	void UpdateUniformBuffer(UniformBuffer* uniform_buffer);

private:
	// ���������̬
	void UpdateCameraPose();
	void HandleMouseEvents();
	void HandleKeyEvents();


public:
	Vec3f position_;					// ���������ռ�λ��
	Vec3f target_;					// ������������ռ�λ��
	Vec3f up_;						// �����up����

	/*
		�������ϵ����
		axis_r��������ָ����Ļ�Ҳ�
		axis_u��������ָ����Ļ�ϲ�
		axis_v��������ָ����Ļ
	*/
	Vec3f axis_r, axis_u, axis_v;

	float fov_;						// ���FOV
	float aspect_;					// �����

	float near_plane_;				// ���ü�ƽ��
	float far_plane_;					// Զ�ü�ƽ��
};

#endif // !CAMERA_H
