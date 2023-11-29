#include <iostream>
#include <fstream>

#include "MoRenderer.h"
#include "win32.h"
#include "model.h"
#include "Texture.h"
#include "Camera.h"

int main() {

	constexpr int width = 600;
	constexpr int height = 800;

	window_init(width, height, "MoRenderer");

	int num_frames = 0;
	float print_time = platform_get_time();
	std::string log_message;


	auto rh = new MoRenderer(width, height);
	rh->SetRenderState(false, true);
	// ����ģ��
	const std::string model_name = "C:/WorkSpace/MoRenderer/models/diablo3_pose.obj";
	auto model = new Model(model_name);

	// ������ͼ
	auto* diffuse_map = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_diffuse.bmp");
	auto* normal_map = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_nm.bmp");
	auto specular_map = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_spec.bmp");

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
	rh->SetVertexShader([&](const int index, ShaderContext& output) -> Vec4f {
		Vec4f vertex = uniform_buffer.mvp_matrix * vs_input[index].positionOS.xyz1();
		// ������λ�ô�ģ�Ϳռ�ת��Ϊ��������ϵ
		const Vec3f position_ws = (uniform_buffer.model_matrix * vs_input[index].positionOS.xyz1()).xyz();

		output.varying_vec3f[VARYING_POSITION_WS] = position_ws;
		output.varying_vec2f[VARYING_TEXCOORD] = vs_input[index].texcoord;
		return vertex;
		});


	// ������ɫ����ʹ��Blinn Phong����ģ��
	rh->SetPixelShader([&](ShaderContext& input) -> Vec4f {
		Vec2f uv = input.varying_vec2f[VARYING_TEXCOORD];

		Vec3f world_normal = (normal_map->Sample2D(uv) * uniform_buffer.normal_matrix).xyz();
		Vec3f world_position = input.varying_vec3f[VARYING_TEXCOORD];

		Vec3f light_dir = vector_normalize(-light_direction);
		Vec3f view_dir = vector_normalize(camera->position_ - world_position);

		//������
		Vec4f base_color = diffuse_map->Sample2D(uv);
		Vec4f diffuse = base_color * Saturate(vector_dot(light_dir, world_normal));

		//�߹�
		//float _Specluar = specular->Sample2D(uv).b * 5;
		float _Specluar = 1.0f;
		Vec3f half_dir = vector_normalize(view_dir + light_dir);
		float intensity = pow(Saturate(vector_dot(world_normal, half_dir)), 20);
		Vec4f specular = base_color * intensity * _Specluar;

		Vec4f color = diffuse + specular;
		return color;
		});

	while (!window->is_close)
	{
		float current_time = platform_get_time();

		camera->HandleInputEvents();
		camera->UpdateUniformBuffer(&uniform_buffer);

		// ��Ⱦģ��
		rh->ClearFrameBuffer();
		for (size_t i = 0; i < model->vertices.size(); i += 3)
		{
			// ����������������룬�� VS ��ȡ
			for (int j = 0; j < 3; j++) {
				vs_input[j].positionOS = model->vertices[i + j].positionOS;
				vs_input[j].texcoord = model->vertices[i + j].texcoord;
				vs_input[j].normal = model->vertices[i + j].normal;
			}

			// ����������
			rh->DrawTriangle();
		}

		// ���ͼ��
		uint8_t* image_data = new uint8_t[width * height * 4];
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				ColorRGBA_32bit color = vector_to_32bit_color(rh->color_buffer_[y][x]);

				//32 bitλͼ�洢˳�򣬴ӵ͵�������ΪBGRA
				image_data[4 * (y * width + x)] = color.b;
				image_data[4 * (y * width + x) + 1] = color.g;
				image_data[4 * (y * width + x) + 2] = color.r;
				image_data[4 * (y * width + x) + 3] = color.a;
			}
		}

		// ���㲢��ʾFPS
		num_frames += 1;
		if (current_time - print_time >= 1) {
			int sum_millis = (int)((current_time - print_time) * 1000);
			int avg_millis = sum_millis / num_frames;

			log_message = "FPS: " + std::to_string(num_frames) + " / " + std::to_string(avg_millis) + " ms";

			num_frames = 0;
			print_time = current_time;
		}

		window_draw(image_data, log_message);
		msg_dispatch();
	}

	return 0;
}
