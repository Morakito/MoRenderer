#include "camera.h"

Camera::Camera(const Vec3f position, const Vec3f& target, Vec3f up, float fov, float aspect) :
	position_(position), target_(target), up_(up), fov_(fov), aspect_(aspect)
{
	near_plane_ = 0.01f;
	far_plane_ = 5.0f;

	window_ = Window::GetInstance();
}

Camera::~Camera() {}


void Camera::UpdateCameraPose()
{
	// �۲������������λ��ָ��Ŀ��λ��
	Vec3f view = position_ - target_;
	float radius = vector_length(view);

	float phi = (float)atan2(view[0], view[2]);				// azimuth angle(��λ��), angle between from_target and z-axis��[-pi, pi]
	float theta = (float)acos(view[1] / radius);			// zenith angle(�춥��), angle between from_target and y-axis, [0, pi]
	float mouseDeltaX = window_->mouse_info_.mouse_delta[0] / window_->width_;
	float mouseDeltaY = window_->mouse_info_.mouse_delta[1] / window_->height_;

	// ������
	if (window_->mouse_buttons_[0])
	{
		float factor = 1.5 * PI;

		phi += mouseDeltaX * factor;
		theta += mouseDeltaY * factor;
		if (theta > PI) theta = PI - EPSILON * 100;
		if (theta < 0)  theta = EPSILON * 100;
	}

	if (window_->mouse_buttons_[1])
	{
		// ����Ҽ�
		float factor = radius * (float)tan(60.0 / 360 * PI) * 2.2;
		Vec3f right = mouseDeltaX * factor * axis_r;
		Vec3f up = mouseDeltaY * factor * axis_u;

		position_ += (-right + up);
		target_ += (-right + up);
	}

	// ������
	if (window_->mouse_buttons_[2])
	{
		radius *= static_cast<float>(pow(0.95, window_->mouse_info_.mouse_wheel_delta));
		window_->mouse_buttons_[2] = 0;
	}

	position_[0] = target_[0] + radius * sin(phi) * sin(theta);
	position_[1] = target_[1] + radius * cos(theta);
	position_[2] = target_[2] + radius * sin(theta) * cos(phi);
}


void Camera::HandleInputEvents()
{
	/*
		�����������ϵ����
		axis_v���۲�����
		axis_r��������ָ����Ļ�Ҳ�
		axis_u��������ָ����Ļ�ϲ�
	*/
	axis_v = vector_normalize(target_ - position_);
	axis_r = vector_normalize(vector_cross(axis_v, up_));
	axis_u = vector_normalize(vector_cross(axis_r, axis_v));

	// ���������¼�
	HandleMouseEvents();
	HandleKeyEvents();
}

void Camera::HandleMouseEvents()
{

	if (window_->mouse_buttons_[0] || window_->mouse_buttons_[1] || window_->mouse_buttons_[2])
	{
		const Vec2f mouse_position = window_->GetMousePosition();
		window_->mouse_info_.mouse_delta = window_->mouse_info_.mouse_position - mouse_position;
		window_->mouse_info_.mouse_position = mouse_position;

		UpdateCameraPose();
	}
}

void Camera::HandleKeyEvents()
{
	const float distance = vector_length(target_ - position_);

	if (window_->keys_['Q'])
	{
		const float factor = distance / window_->width_ * 200.0f;
		position_ += -0.05f * axis_v * factor;
	}
	if (window_->keys_['E'])
	{
		position_ += 0.05f * axis_v;
	}
	if (window_->keys_[VK_UP] || window_->keys_['W'])
	{
		position_ += 0.05f * axis_u;
		target_ += 0.05f * axis_u;
	}
	if (window_->keys_[VK_DOWN] || window_->keys_['S'])
	{
		position_ += -0.05f * axis_u;
		target_ += -0.05f * axis_u;
	}
	if (window_->keys_[VK_LEFT] || window_->keys_['A'])
	{
		position_ += -0.05f * axis_r;
		target_ += -0.05f * axis_r;
	}
	if (window_->keys_[VK_RIGHT] || window_->keys_['D'])
	{
		position_ += 0.05f * axis_r;
		target_ += 0.05f * axis_r;
	}
	if (window_->keys_[VK_ESCAPE])
	{
		window_->is_close_ = true;
	}
}

void Camera::UpdateUniformBuffer(UniformBuffer* uniform_buffer)
{
	uniform_buffer->model_matrix = matrix_set_scale(1, 1, 1);
	uniform_buffer->view_matrix = matrix_look_at(position_, target_, up_);
	uniform_buffer->proj_matrix = matrix_set_perspective(fov_, aspect_, near_plane_, far_plane_);;

	uniform_buffer->CalculateRestMatrix();
}


