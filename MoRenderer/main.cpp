#include <iostream>
#include <fstream>
#include <set>

#include "MoRenderer.h"
#include "Window.h"
#include "model.h"
#include "Camera.h"

int main() {

	constexpr int width = 800;
	constexpr int height = 600;

	Window* window = Window::GetInstance();
	window->WindowInit(width, height, "MoRenderer");

	// ����ģ��
	std::string file_path, file_name, texture_format;

	file_path = "C:/WorkSpace/MoRenderer/models/helmet";
	file_name = "helmet";
	texture_format = ".tga";

	const auto model = new Model(file_path, file_name, texture_format);
	const std::string model_message = "vertex count: " + std::to_string(model->vertex_number_) + "  face count: " + std::to_string(model->face_number_) + "\n";
	window->SetLogMessage("model_message", model_message);

	// ��������͹�Դ
	const Vec3f camera_position = { 0, 0, 2 };				// ���λ��
	const Vec3f camera_target = { 0, 0, 0 };					// ��������λ��
	const Vec3f camera_up = { 0, 1, 0 };						// ������ϵ�λ��
	constexpr float fov = 90.0f;
	auto* camera = new Camera(camera_position, camera_target, camera_up, fov, static_cast<float>(width) / height);

	const auto blinn_phong_shader = new BlinnPhongShader();

	const auto uniform_buffer = new UniformBuffer();
	uniform_buffer->model_matrix = matrix_set_rotate(1.0f, 0.0f, 0.0f, -kPi * 0.5f) * matrix_set_scale(1, 1, 1);
	uniform_buffer->view_matrix = matrix_look_at(camera_position, camera_target, camera_up);
	uniform_buffer->proj_matrix = matrix_set_perspective(fov, camera->aspect_, camera->near_plane_, camera->near_plane_);
	uniform_buffer->CalculateRestMatrix();

	uniform_buffer->light_direction = { 0, -0.5f, -2 };
	uniform_buffer->light_color = { 1.0f, 1.0f, 1.0f,1.0f };
	uniform_buffer->camera_position = camera->position_;

	blinn_phong_shader->uniform_buffer_ = uniform_buffer;
	blinn_phong_shader->model_ = model;

	// ��ʼ����Ⱦ��
	const auto mo_renderer = new MoRenderer(width, height);
	mo_renderer->SetRenderState(false, true);
	mo_renderer->SetVertexShader(blinn_phong_shader->vertex_shader_);
	mo_renderer->SetPixelShader(blinn_phong_shader->pixel_shader_);


	while (!window->is_close_)
	{
		camera->HandleInputEvents();
		camera->UpdateUniformBuffer(blinn_phong_shader->uniform_buffer_);

		// ��Ⱦģ��
		mo_renderer->ClearFrameBuffer();
		for (size_t i = 0; i < model->attributes_.size(); i += 3)
		{
			// ����������������룬�� VS ��ȡ
			for (int j = 0; j < 3; j++) {
				blinn_phong_shader->attributes_[j].position_os = model->attributes_[i + j].position_os;
				blinn_phong_shader->attributes_[j].texcoord = model->attributes_[i + j].texcoord;
				blinn_phong_shader->attributes_[j].normal_os = model->attributes_[i + j].normal_os;
				blinn_phong_shader->attributes_[j].tangent_os = model->attributes_[i + j].tangent_os;
			}


			// ����������
			mo_renderer->DrawTriangle();
		}

		window->WindowDisplay(mo_renderer->color_buffer_);
	}

	return 0;
}
