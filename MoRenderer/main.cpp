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
	const std::string model_name = "C:/WorkSpace/MoRenderer/models/diablo3_pose.obj";
	const auto model = new Model(model_name);
	auto* diffuse_map = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_diffuse.bmp");
	auto* normal_map = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_nm.bmp");
	auto* specular_map = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_spec.bmp");


	std::string model_message = "vertex count: " + std::to_string(model->vertex_number_) + "  face count: " + std::to_string(model->face_number_) + "\n";
	log_messages["model_message"] = model_message;

	// ��������͹�Դ
	Vec3f camera_position = { 0, 0, 2 };					// ���λ��
	Vec3f camera_target = { 0, 0, 0 };					// ��������λ��
	Vec3f camera_up = { 0, 1, 0 };						// ������ϵ�λ��
	Vec3f light_direction = { 0, -0.5f, -2 };				// ���շ���
	float fov = 90.0f;


	auto* camera = new Camera(camera_position, camera_target, camera_up, fov, static_cast<float>(width) / height);

	BlinnPhongShader blinn_phong_shader;
	blinn_phong_shader.uniform_buffer_->model_matrix = matrix_set_scale(1, 1, 1);
	blinn_phong_shader.uniform_buffer_->view_matrix = matrix_look_at(camera_position, camera_target, camera_up);
	blinn_phong_shader.uniform_buffer_->proj_matrix = matrix_set_perspective(fov, camera->aspect_, camera->near_plane_, camera->near_plane_);
	blinn_phong_shader.uniform_buffer_->CalculateRestMatrix();


	mo_renderer->SetVertexShader(blinn_phong_shader.vertex_shader_);

	// ������ɫ����ʹ��Blinn Phong����ģ��
	mo_renderer->SetPixelShader([&](ShaderContext& input) -> Vec4f {
		Vec2f uv = input.varying_vec2f[BlinnPhongShader::VARYING_TEXCOORD];

		Vec3f world_normal = (normal_map->Sample2D(uv) * blinn_phong_shader.uniform_buffer_->normal_matrix).xyz();
		Vec3f world_position = input.varying_vec3f[BlinnPhongShader::VARYING_TEXCOORD];

		Vec3f light_dir = vector_normalize(-light_direction);
		Vec3f view_dir = vector_normalize(camera->position_ - world_position);

		//������
		Vec4f base_color = diffuse_map->Sample2D(uv);
		Vec4f diffuse = base_color * Saturate(vector_dot(light_dir, world_normal));

		//�߹�
		float specular_scale = specular_map->Sample2D(uv).b * 5;
		Vec3f half_dir = vector_normalize(view_dir + light_dir);
		float intensity = pow(Saturate(vector_dot(world_normal, half_dir)), 20);
		Vec4f specular = base_color * intensity * specular_scale;

		Vec4f color = diffuse + specular;
		return color;
		});

	while (!window->is_close_)
	{
		float current_time = Window::PlatformGetTime();

		camera->HandleInputEvents();
		camera->UpdateUniformBuffer(blinn_phong_shader.uniform_buffer_);

		// ��Ⱦģ��
		mo_renderer->ClearFrameBuffer();
		for (size_t i = 0; i < model->vertices_.size(); i += 3)
		{
			// ����������������룬�� VS ��ȡ
			for (int j = 0; j < 3; j++) {
				blinn_phong_shader.vs_input_[j].position_os = model->vertices_[i + j].position_os;
				blinn_phong_shader.vs_input_[j].texcoord = model->vertices_[i + j].texcoord;
				blinn_phong_shader.vs_input_[j].normal = model->vertices_[i + j].normal;
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
