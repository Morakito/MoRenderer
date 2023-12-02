#include <iostream>
#include <fstream>
#include <set>

#include "MoRenderer.h"
#include "Window.h"
#include "model.h"
#include "Texture.h"
#include "Camera.h"

int main() {

	constexpr int width = 600;
	constexpr int height = 800;

	Window* window = Window::GetInstance();
	window->WindowInit(width, height, "MoRenderer");

	int num_frames = 0;
	float print_time = Window::PlatformGetTime();
	std::map<std::string, std::string> log_messages;
	log_messages["fps_message"] = " ";
	log_messages["model_message"] = " ";

	const auto mo_renderer = new MoRenderer(width, height);
	mo_renderer->SetRenderState(false, true);


	// ����ģ��
	std::string file_path = "C:/WorkSpace/MoRenderer/models/diablo3";
	std::string file_name = "diablo3";
	std::string texture_format = ".bmp";
	const auto model = new Model(file_path, file_name, texture_format);


	std::string model_message = "vertex count: " + std::to_string(model->vertex_number_) + "  face count: " + std::to_string(model->face_number_) + "\n";
	log_messages["model_message"] = model_message;

	// ��������͹�Դ
	Vec3f camera_position = { 0, 0, 2 };					// ���λ��
	Vec3f camera_target = { 0, 0, 0 };					// ��������λ��
	Vec3f camera_up = { 0, 1, 0 };						// ������ϵ�λ��
	float fov = 90.0f;
	auto* camera = new Camera(camera_position, camera_target, camera_up, fov, static_cast<float>(width) / height);

	auto blinn_phong_shader = new BlinnPhongShader();

	auto uniform_buffer = new UniformBuffer();

	uniform_buffer->model_matrix = matrix_set_scale(1, 1, 1);
	uniform_buffer->view_matrix = matrix_look_at(camera_position, camera_target, camera_up);
	uniform_buffer->proj_matrix = matrix_set_perspective(fov, camera->aspect_, camera->near_plane_, camera->near_plane_);
	uniform_buffer->CalculateRestMatrix();

	uniform_buffer->light_direction = { 0, -0.5f, -2 };
	uniform_buffer->light_color = { 1.0f, 1.0f, 1.0f,1.0f };
	uniform_buffer->camera_position = camera->position_;

	blinn_phong_shader->uniform_buffer_ = uniform_buffer;
	blinn_phong_shader->model_ = model;

	mo_renderer->SetVertexShader(blinn_phong_shader->vertex_shader_);
	mo_renderer->SetPixelShader(blinn_phong_shader->pixel_shader_);


	while (!window->is_close_)
	{
		float current_time = Window::PlatformGetTime();

		camera->HandleInputEvents();
		camera->UpdateUniformBuffer(blinn_phong_shader->uniform_buffer_);

		// ��Ⱦģ��
		mo_renderer->ClearFrameBuffer();
		for (size_t i = 0; i < model->vertices_.size(); i += 3)
		{
			// ����������������룬�� VS ��ȡ
			for (int j = 0; j < 3; j++) {
				blinn_phong_shader->attributes_[j].position_os = model->vertices_[i + j].position_os;
				blinn_phong_shader->attributes_[j].texcoord = model->vertices_[i + j].texcoord;
				blinn_phong_shader->attributes_[j].normal = model->vertices_[i + j].normal;
			}

			// ����������
			mo_renderer->DrawTriangle();
		}



		// ���㲢��ʾFPS
		num_frames += 1;
		if (current_time - print_time >= 1) {
			int sum_millis = static_cast<int>((current_time - print_time) * 1000);
			int avg_millis = sum_millis / num_frames;

			std::string fps_message = "FPS: " + std::to_string(num_frames) + " / " + std::to_string(avg_millis) + " ms";

			log_messages["fps_message"] = fps_message;
			num_frames = 0;
			print_time = current_time;
		}

		// ��ʾͼ��
		window->WindowDisplay(mo_renderer->color_buffer_, log_messages);
		Window::MessageDispatch();

	}

	return 0;
}
