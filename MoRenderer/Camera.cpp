#include "camera.h"

#include "win32.h"


Camera::Camera(Vec3f _position, Vec3f _target, Vec3f _up, float _fov, float _aspect) :
	position(_position), target(_target), up(_up), fov(_fov), aspect(_aspect)
{
	nearPlane = 1.0f;
	farPlane = 500.0f;
}

Camera::~Camera() {}


void Camera::UpdataCameraPose()
{
	// �۲������������λ��ָ��Ŀ��λ��
	Vec3f view = position - target;
	float radius = vector_length(view);

	float phi = (float)atan2(view[0], view[2]);				// azimuth angle(��λ��), angle between from_target and z-axis��[-pi, pi]
	float theta = (float)acos(view[1] / radius);			// zenith angle(�춥��), angle between from_target and y-axis, [0, pi]
	float mouseDeltaX = window->mouse_info.mouseDelta[0] / window->width;
	float mouseDeltaY = window->mouse_info.mouseDelta[1] / window->height;

	// ������
	if (window->mouseButtons[0])
	{
		float factor = 1.5 * PI;

		phi += mouseDeltaX * factor;
		theta += mouseDeltaY * factor;
		if (theta > PI) theta = PI - EPSILON * 100;
		if (theta < 0)  theta = EPSILON * 100;
	}

	if (window->mouseButtons[1])
	{
		// ����Ҽ�
		float factor = radius * (float)tan(60.0 / 360 * PI) * 2.2;
		Vec3f right = mouseDeltaX * factor * axisX;
		Vec3f up = mouseDeltaY * factor * axisY;

		position += (right - up);
		target += (right - up);
	}

	// ������
	if (window->mouseButtons[2])
	{
		radius *= (float)pow(0.95, window->mouse_info.mouseWheelDelta);
		window->mouseButtons[2] = 0;
	}

	position[0] = target[0] + radius * sin(phi) * sin(theta);
	position[1] = target[1] + radius * cos(theta);
	position[2] = target[2] + radius * sin(theta) * cos(phi);
}


void Camera::HandleInputEvents()
{
	/*
		�����������ϵ����
		axisX��������ָ����Ļ�Ҳ�
		axisY��������ָ����Ļ�ϲ�
		axisZ��������ָ����Ļ
	*/
	axisZ = vector_normalize(position - target);
	axisX = vector_normalize(vector_cross(axisZ, up));
	axisY = vector_normalize(vector_cross(axisX, axisZ));

	// ���������¼�
	HandleMouseEvents();
	HandleKeyEvents();
}

void Camera::HandleMouseEvents()
{

	if (window->mouseButtons[0] || window->mouseButtons[1] || window->mouseButtons[2])
	{
		Vec2f mousePosition = GetMousePosition();
		window->mouse_info.mouseDelta = window->mouse_info.mousePosition - mousePosition;
		window->mouse_info.mousePosition = mousePosition;

		UpdataCameraPose();
	}
}

void Camera::HandleKeyEvents()
{
	float distance = vector_length(target - position);

	if (window->keys['Q'])
	{
		float factor = distance / window->width * 200.0f;
		position += -0.05f * axisZ * factor;
	}
	if (window->keys['E'])
	{
		position += 0.05f * axisZ;
	}
	if (window->keys[VK_UP] || window->keys['W'])
	{
		position += 0.05f * axisY;
		target += 0.05f * axisY;
	}
	if (window->keys[VK_DOWN] || window->keys['S'])
	{
		position += -0.05f * axisY;
		target += -0.05f * axisY;
	}
	if (window->keys[VK_LEFT] || window->keys['A'])
	{
		position += -0.05f * axisX;
		target += -0.05f * axisX;
	}
	if (window->keys[VK_RIGHT] || window->keys['D'])
	{
		position += 0.05f * axisX;
		target += 0.05f * axisX;
	}
	if (window->keys[VK_ESCAPE])
	{
		window->is_close = 1;
	}
}

void Camera::UpdateUniformBuffer(UniformBuffer* uniformbuffer)
{
	uniformbuffer->modelMatrix = matrix_set_scale(1, 1, 1);
	uniformbuffer->viewMatrix = matrix_set_lookat(position, target, up);
	uniformbuffer->projMatrix = matrix_set_perspective(fov, aspect, nearPlane, farPlane);;

	uniformbuffer->CalculateRestMatrix();
}


