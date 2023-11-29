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
	float print_time = window->PlatformGetTime();
	std::map<std::string, std::string> log_messages;
	log_messages["fps_message"] = " ";
	log_messages["model_message"] = " ";

	const auto mo_renderer = new MoRenderer(width, height);
	mo_renderer->SetRenderState(false, true);
	// ����ģ��
	const std::string model_name = "C:/WorkSpace/MoRenderer/models/diablo3_pose.obj";
	const auto model = new Model(model_name);


	// ������ͼ
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
	UniformBuffer uniform_buffer{};
	uniform_buffer.model_matrix = matrix_set_scale(1, 1, 1);
	uniform_buffer.view_matrix = matrix_look_at(camera_position, camera_target, camera_up);
	uniform_buffer.proj_matrix = matrix_set_perspective(fov, camera->aspect_, camera->near_plane_, camera->near_plane_);
	uniform_buffer.CalculateRestMatrix();

	// ��������
	Vertex vs_input[3];
	enum VaryingAttributes
	{
		VARYING_TEXCOORD = 0,		// ��������
		VARYING_POSITION_WS = 1		// ��������
	};

	// ������ɫ��
	mo_renderer->SetVertexShader([&](const int index, ShaderContext& output) -> Vec4f {
		Vec4f vertex = uniform_buffer.mvp_matrix * vs_input[index].position_os.xyz1();
		// ������λ�ô�ģ�Ϳռ�ת��Ϊ��������ϵ
		const Vec3f position_ws = (uniform_buffer.model_matrix * vs_input[index].position_os.xyz1()).xyz();

		output.varying_vec3f[VARYING_POSITION_WS] = position_ws;
		output.varying_vec2f[VARYING_TEXCOORD] = vs_input[index].texcoord;
		return vertex;
		});


	// ������ɫ����ʹ��Blinn Phong����ģ��
	mo_renderer->SetPixelShader([&](ShaderContext& input) -> Vec4f {
		Vec2f uv = input.varying_vec2f[VARYING_TEXCOORD];

		Vec3f world_normal = (normal_map->Sample2D(uv) * uniform_buffer.normal_matrix).xyz();
		Vec3f world_position = input.varying_vec3f[VARYING_TEXCOORD];

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
		float current_time = window->PlatformGetTime();

		camera->HandleInputEvents();
		camera->UpdateUniformBuffer(&uniform_buffer);

		// ��Ⱦģ��
		mo_renderer->ClearFrameBuffer();
		for (size_t i = 0; i < model->vertices_.size(); i += 3)
		{
			// ����������������룬�� VS ��ȡ
			for (int j = 0; j < 3; j++) {
				vs_input[j].position_os = model->vertices_[i + j].position_os;
				vs_input[j].texcoord = model->vertices_[i + j].texcoord;
				vs_input[j].normal = model->vertices_[i + j].normal;
			}

			// ����������
			mo_renderer->DrawTriangle();
		}



		// ���㲢��ʾFPS
		num_frames += 1;
		if (current_time - print_time >= 1) {
			int sum_millis = (int)((current_time - print_time) * 1000);
			int avg_millis = sum_millis / num_frames;

			std::string fps_message = "FPS: " + std::to_string(num_frames) + " / " + std::to_string(avg_millis) + " ms";
			
			log_messages["fps_message"] = fps_message;
			num_frames = 0;
			print_time = current_time;
		}

		// ��ʾͼ��
		window->WindowDisplay(mo_renderer->color_buffer_, log_messages);
		window->MessageDispatch();

	}

	return 0;
}
