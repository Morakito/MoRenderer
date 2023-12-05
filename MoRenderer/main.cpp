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

	// 加载模型
	std::string file_path, file_name, texture_format;

	file_path = "C:/WorkSpace/MoRenderer/models/helmet";
	file_name = "helmet";
	texture_format = ".tga";

	//file_path = "C:/WorkSpace/MoRenderer/models/Cerberus";
	//file_name = "Cerberus";
	//texture_format = ".tga";


	const auto model = new Model(file_path, file_name, texture_format);
	const std::string model_message = "vertex count: " + std::to_string(model->vertex_number_) + "  face count: " + std::to_string(model->face_number_) + "\n";
	window->SetLogMessage("model_message", model_message);

	// 设置相机和光源
	const Vec3f camera_position = { 0, 0, 2 };				// 相机位置
	const Vec3f camera_target = { 0, 0, 0 };					// 相机看向的位置
	const Vec3f camera_up = { 0, 1, 0 };						// 相机向上的位置
	constexpr float fov = 90.0f;
	auto* camera = new Camera(camera_position, camera_target, camera_up, fov, static_cast<float>(width) / height);


	const auto uniform_buffer = new UniformBuffer();
	uniform_buffer->model_matrix = matrix_set_rotate(1.0f, 0.0f, 0.0f, -kPi * 0.5f) * matrix_set_scale(1, 1, 1);
	uniform_buffer->view_matrix = matrix_look_at(camera_position, camera_target, camera_up);
	uniform_buffer->proj_matrix = matrix_set_perspective(fov, camera->aspect_, camera->near_plane_, camera->near_plane_);
	uniform_buffer->CalculateRestMatrix();

	uniform_buffer->light_direction = { 0, 2, -2 };
	uniform_buffer->light_color = Vec3f(1.0f);
	uniform_buffer->camera_position = camera->position_;


	const auto blinn_phong_shader = new BlinnPhongShader();
	blinn_phong_shader->uniform_buffer_ = uniform_buffer;
	blinn_phong_shader->model_ = model;

	const auto pbr_shader = new PBRShader();
	pbr_shader->uniform_buffer_ = uniform_buffer;
	pbr_shader->model_ = model;

	//constexpr ShaderType current_shader_type = kBlinnPhongShader;
	constexpr ShaderType current_shader_type = kPbrShader;

	// 初始化渲染器
	const auto mo_renderer = new MoRenderer(width, height);
	mo_renderer->SetRenderState(false, true);
	while (!window->is_close_)
	{
		switch (current_shader_type)
		{
		case kBlinnPhongShader:
			mo_renderer->SetVertexShader(blinn_phong_shader->vertex_shader_);
			mo_renderer->SetPixelShader(blinn_phong_shader->pixel_shader_);
			camera->HandleInputEvents();
			camera->UpdateUniformBuffer(blinn_phong_shader->uniform_buffer_);

			blinn_phong_shader->HandleKeyEvents();
			break;
		case kPbrShader:
			mo_renderer->SetVertexShader(pbr_shader->vertex_shader_);
			mo_renderer->SetPixelShader(pbr_shader->pixel_shader_);
			camera->HandleInputEvents();
			camera->UpdateUniformBuffer(pbr_shader->uniform_buffer_);

			pbr_shader->HandleKeyEvents();
			break;
		}


		// 渲染模型
		mo_renderer->ClearFrameBuffer();
		for (size_t i = 0; i < model->attributes_.size(); i += 3)
		{
			// 设置三个顶点的输入，供 VS 读取
			for (int j = 0; j < 3; j++) {
				switch (current_shader_type)
				{
				case kBlinnPhongShader:
					blinn_phong_shader->attributes_[j].position_os = model->attributes_[i + j].position_os;
					blinn_phong_shader->attributes_[j].texcoord = model->attributes_[i + j].texcoord;
					blinn_phong_shader->attributes_[j].normal_os = model->attributes_[i + j].normal_os;
					blinn_phong_shader->attributes_[j].tangent_os = model->attributes_[i + j].tangent_os;
					break;
				case kPbrShader:
					pbr_shader->attributes_[j].position_os = model->attributes_[i + j].position_os;
					pbr_shader->attributes_[j].texcoord = model->attributes_[i + j].texcoord;
					pbr_shader->attributes_[j].normal_os = model->attributes_[i + j].normal_os;
					pbr_shader->attributes_[j].tangent_os = model->attributes_[i + j].tangent_os;
					break;
				}
			}
			// 绘制三角形
			mo_renderer->DrawTriangle();
		}

		window->WindowDisplay(mo_renderer->color_buffer_);
	}

	return 0;
}
