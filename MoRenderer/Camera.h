#ifndef CAMERA_H
#define CAMERA_H

#include "math.h"

#include "Shader.h"
#include "Window.h"

class Camera
{
public:
	Camera(const Vec3f& position, const Vec3f& target, const Vec3f& up, float fov, float aspect);
	~Camera();

	// ���������¼�
	void HandleInputEvents();

	// ����uniform buffer�еľ���
	void UpdateUniformBuffer(UniformBuffer* uniform_buffer) const;

private:
	// ���������̬
	void UpdateCameraPose();
	void HandleMouseEvents();
	void HandleKeyEvents();


public:
	Window* window_;
	Vec3f position_, origin_position_;	// ���������ռ�λ��
	Vec3f target_, origin_target_;		// ������������ռ�λ��
	Vec3f up_;							// �����up����

	/*
		�������ϵ����
		axis_r��������ָ����Ļ�Ҳ�
		axis_u��������ָ����Ļ�ϲ�
		axis_v��������ָ����Ļ
	*/
	Vec3f axis_r_, axis_u_, axis_v_;

	float fov_;						// ���FOV
	float aspect_;					// �����

	float near_plane_;				// ���ü�ƽ��
	float far_plane_;				// Զ�ü�ƽ��
};

#endif // !CAMERA_H
