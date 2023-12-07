#include <iostream>
#include <fstream>
#include <set>

#include "MoRenderer.h"
#include "Window.h"
#include "model.h"
#include "Camera.h"

#include  "cmgen.h"

int main1()
{
	GenerateCubeMap();

	return  0;
}

int main() {

	constexpr int width = 800;
	constexpr int height = 600;

	Window* window = Window::GetInstance();
	window->WindowInit(width, height, "MoRenderer");

#pragma region 外部资源加载

	// 加载模型
	std::string file_path, file_name, texture_format;
	file_path = "C:/WorkSpace/MoRenderer/assets/helmet";
	file_name = "helmet";
	texture_format = ".tga";

	const auto model = new Model(file_path, file_name, texture_format);
	const std::string model_message = "vertex count: " + std::to_string(model->vertex_number_) + "  face count: " + std::to_string(model->face_number_) + "\n";
	window->SetLogMessage("model_message", model_message);

	// 加载环境贴图
	std::string skybox_folder = "C:/WorkSpace/MoRenderer/assets/spruit_sunrise/spruit_sunrise/";
	auto* skybox_cubemap = new CubeMap(skybox_folder, CubeMap::kSkybox);
	auto* irradiance_cubemap = new CubeMap(skybox_folder, CubeMap::kIrradianceMap);
	auto* specular_cubemap = new SpecularCubeMap(skybox_folder, CubeMap::kSpecularMap);
	auto* brdf_lut = new Texture(skybox_folder + "brdf_lut.hdr");

#pragma endregion

#pragma region 配置UniformBuffer
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

	uniform_buffer->light_direction = { 0, 4, -2 };
	uniform_buffer->light_color = Vec3f(1.0f);
	uniform_buffer->camera_position = camera->position_;
#pragma endregion

#pragma region 配置Renderer
	const auto blinn_phong_shader = new BlinnPhongShader();
	blinn_phong_shader->uniform_buffer_ = uniform_buffer;
	blinn_phong_shader->model_ = model;

	const auto pbr_shader = new PBRShader();
	pbr_shader->uniform_buffer_ = uniform_buffer;
	pbr_shader->model_ = model;
	pbr_shader->irradiance_cubemap_ = irradiance_cubemap;
	pbr_shader->specular_cubemap_ = specular_cubemap;
	pbr_shader->brdf_lut_ = brdf_lut;

	const auto skybox_shader = new SkyBoxShader();
	skybox_shader->uniform_buffer_ = uniform_buffer;
	skybox_shader->skybox_cubemap_ = skybox_cubemap;

	//constexpr ShaderType current_shader_type = kBlinnPhongShader;
	constexpr ShaderType current_shader_type = kPbrShader;

	// 初始化渲染器
	const auto mo_renderer = new MoRenderer(width, height);
	mo_renderer->SetRenderState(false, true);

	bool is_render_skybox = true;
	bool is_render_model = true;

#pragma endregion

#pragma region RenderLoop

	while (!window->is_close_)
	{
		mo_renderer->ClearFrameBuffer();
		camera->HandleInputEvents();

		if (is_render_model) {
			switch (current_shader_type)
			{
			case kBlinnPhongShader:
				mo_renderer->SetVertexShader(blinn_phong_shader->vertex_shader_);
				mo_renderer->SetPixelShader(blinn_phong_shader->pixel_shader_);
				camera->UpdateUniformBuffer(blinn_phong_shader->uniform_buffer_);

				blinn_phong_shader->HandleKeyEvents();
				break;
			case kPbrShader:
				mo_renderer->SetVertexShader(pbr_shader->vertex_shader_);
				mo_renderer->SetPixelShader(pbr_shader->pixel_shader_);
				camera->UpdateUniformBuffer(pbr_shader->uniform_buffer_);

				pbr_shader->HandleKeyEvents();
				break;
			}

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
				mo_renderer->DrawMesh();
			}
		}

		if (is_render_skybox) {
			// 渲染skybox
			mo_renderer->SetVertexShader(skybox_shader->vertex_shader_);
			mo_renderer->SetPixelShader(skybox_shader->pixel_shader_);
			camera->UpdateSkyBoxUniformBuffer(skybox_shader->uniform_buffer_);

			camera->HandleInputEvents();
			camera->UpdateSkyboxMesh(skybox_shader);
			int num = skybox_shader->plane_vertex_.size();
			for (size_t i = 0; i < num - 2; i++)
			{
				skybox_shader->attributes_[0].position_os = skybox_shader->plane_vertex_[0];
				skybox_shader->attributes_[1].position_os = skybox_shader->plane_vertex_[i + 1];
				skybox_shader->attributes_[2].position_os = skybox_shader->plane_vertex_[i + 2];

				mo_renderer->DrawSkybox();
			}
		}
		window->WindowDisplay(mo_renderer->color_buffer_);
	}

#pragma endregion


	return 0;
}
