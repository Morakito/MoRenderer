#include "camera.h"

#include "win32.h"


Camera::Camera(Vec3f position, Vec3f target, Vec3f up, float fov, float aspect) :
	position_(position), target_(target), up_(up), fov_(fov), aspect_(aspect)
{
	near_plane_ = 0.01f;
	far_plane_ = 5.0f;
}

Camera::~Camera() {}


void Camera::UpdateCameraPose()
{
	// 观察向量：从相机位置指向目标位置
	Vec3f view = position_ - target_;
	float radius = vector_length(view);

	float phi = (float)atan2(view[0], view[2]);				// azimuth angle(方位角), angle between from_target and z-axis，[-pi, pi]
	float theta = (float)acos(view[1] / radius);			// zenith angle(天顶角), angle between from_target and y-axis, [0, pi]
	float mouseDeltaX = window->mouse_info.mouse_delta[0] / window->width;
	float mouseDeltaY = window->mouse_info.mouse_delta[1] / window->height;

	// 鼠标左键
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
		// 鼠标右键
		float factor = radius * (float)tan(60.0 / 360 * PI) * 2.2;
		Vec3f right = mouseDeltaX * factor * axis_r;
		Vec3f up = mouseDeltaY * factor * axis_u;

		position_ += (-right + up);
		target_ += (-right + up);
	}

	// 鼠标滚轮
	if (window->mouseButtons[2])
	{
		radius *= static_cast<float>(pow(0.95, window->mouse_info.mouse_wheel_delta));
		window->mouseButtons[2] = 0;
	}

	position_[0] = target_[0] + radius * sin(phi) * sin(theta);
	position_[1] = target_[1] + radius * cos(theta);
	position_[2] = target_[2] + radius * sin(theta) * cos(phi);
}


void Camera::HandleInputEvents()
{
	/*
		计算相机坐标系的轴
		axis_v：观察向量
		axis_r：正方向指向屏幕右侧
		axis_u：正方向指向屏幕上侧
	*/
	axis_v = vector_normalize(target_-position_ );
	axis_r = vector_normalize(vector_cross(axis_v, up_));
	axis_u = vector_normalize(vector_cross(axis_r, axis_v));

	// 处理输入事件
	HandleMouseEvents();
	HandleKeyEvents();
}

void Camera::HandleMouseEvents()
{

	if (window->mouseButtons[0] || window->mouseButtons[1] || window->mouseButtons[2])
	{
		const Vec2f mouse_position = GetMousePosition();
		window->mouse_info.mouse_delta = window->mouse_info.mouse_position - mouse_position;
		window->mouse_info.mouse_position = mouse_position;

		UpdateCameraPose();
	}
}

void Camera::HandleKeyEvents()
{
	const float distance = vector_length(target_ - position_);

	if (window->keys['Q'])
	{
		const float factor = distance / window->width * 200.0f;
		position_ += -0.05f * axis_v * factor;
	}
	if (window->keys['E'])
	{
		position_ += 0.05f * axis_v;
	}
	if (window->keys[VK_UP] || window->keys['W'])
	{
		position_ += 0.05f * axis_u;
		target_ += 0.05f * axis_u;
	}
	if (window->keys[VK_DOWN] || window->keys['S'])
	{
		position_ += -0.05f * axis_u;
		target_ += -0.05f * axis_u;
	}
	if (window->keys[VK_LEFT] || window->keys['A'])
	{
		position_ += -0.05f * axis_r;
		target_ += -0.05f * axis_r;
	}
	if (window->keys[VK_RIGHT] || window->keys['D'])
	{
		position_ += 0.05f * axis_r;
		target_ += 0.05f * axis_r;
	}
	if (window->keys[VK_ESCAPE])
	{
		window->is_close = 1;
	}
}

void Camera::UpdateUniformBuffer(UniformBuffer* uniform_buffer)
{
	uniform_buffer->model_matrix = matrix_set_scale(1, 1, 1);
	uniform_buffer->view_matrix = matrix_look_at(position_, target_, up_);
	uniform_buffer->proj_matrix = matrix_set_perspective(fov_, aspect_, near_plane_, far_plane_);;

	uniform_buffer->CalculateRestMatrix();
}


