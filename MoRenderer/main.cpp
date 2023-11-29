#include <iostream>
#include <fstream>

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
	std::string log_message;


	auto rh = new MoRenderer(width, height);
	rh->SetRenderState(false, true);
	// 加载模型
	const std::string model_name = "C:/WorkSpace/MoRenderer/models/diablo3_pose.obj";
	auto model = new Model(model_name);

	// 加载贴图
	auto* diffuse_map = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_diffuse.bmp");
	auto* normal_map = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_nm.bmp");
	auto* specular_map = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_spec.bmp");

	// 设置相机和光源
	Vec3f camera_position = { 0, 0, 2 };					// 相机位置
	Vec3f camera_target = { 0, 0, 0 };					// 相机看向的位置
	Vec3f camera_up = { 0, 1, 0 };						// 相机向上的位置
	Vec3f light_direction = { 0, -0.5f, -2 };				// 光照方向
	float fov = 90.0f;


	auto* camera = new Camera(camera_position, camera_target, camera_up, fov, static_cast<float>(width) / height);
	UniformBuffer uniform_buffer{};
	uniform_buffer.model_matrix = matrix_set_scale(1, 1, 1);
	uniform_buffer.view_matrix = matrix_look_at(camera_position, camera_target, camera_up);
	uniform_buffer.proj_matrix = matrix_set_perspective(fov, camera->aspect_, camera->near_plane_, camera->near_plane_);
	uniform_buffer.CalculateRestMatrix();

	// 顶点属性
	Vertex vs_input[3];
	enum VaryingAttributes
	{
		VARYING_TEXCOORD = 0,		// 纹理坐标
		VARYING_POSITION_WS = 1		// 世界坐标
	};

	// 顶点着色器
	rh->SetVertexShader([&](const int index, ShaderContext& output) -> Vec4f {
		Vec4f vertex = uniform_buffer.mvp_matrix * vs_input[index].positionOS.xyz1();
		// 将顶点位置从模型空间转换为世界坐标系
		const Vec3f position_ws = (uniform_buffer.model_matrix * vs_input[index].positionOS.xyz1()).xyz();

		output.varying_vec3f[VARYING_POSITION_WS] = position_ws;
		output.varying_vec2f[VARYING_TEXCOORD] = vs_input[index].texcoord;
		return vertex;
		});


	// 像素着色器：使用Blinn Phong光照模型
	rh->SetPixelShader([&](ShaderContext& input) -> Vec4f {
		Vec2f uv = input.varying_vec2f[VARYING_TEXCOORD];

		Vec3f world_normal = (normal_map->Sample2D(uv) * uniform_buffer.normal_matrix).xyz();
		Vec3f world_position = input.varying_vec3f[VARYING_TEXCOORD];

		Vec3f light_dir = vector_normalize(-light_direction);
		Vec3f view_dir = vector_normalize(camera->position_ - world_position);

		//漫反射
		Vec4f base_color = diffuse_map->Sample2D(uv);
		Vec4f diffuse = base_color * Saturate(vector_dot(light_dir, world_normal));

		//高光
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

		// 渲染模型
		rh->ClearFrameBuffer();
		for (size_t i = 0; i < model->vertices.size(); i += 3)
		{
			// 设置三个顶点的输入，供 VS 读取
			for (int j = 0; j < 3; j++) {
				vs_input[j].positionOS = model->vertices[i + j].positionOS;
				vs_input[j].texcoord = model->vertices[i + j].texcoord;
				vs_input[j].normal = model->vertices[i + j].normal;
			}

			// 绘制三角形
			rh->DrawTriangle();
		}

		// 计算并显示FPS
		num_frames += 1;
		if (current_time - print_time >= 1) {
			int sum_millis = (int)((current_time - print_time) * 1000);
			int avg_millis = sum_millis / num_frames;

			log_message = "FPS: " + std::to_string(num_frames) + " / " + std::to_string(avg_millis) + " ms";

			num_frames = 0;
			print_time = current_time;
		}

		// 显示图像
		window->WindowDisplay(rh->color_buffer_, log_message);
		window->MessageDispatch();
	}

	return 0;
}
