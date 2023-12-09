#include <iostream>
#include <fstream>
#include <set>

#include "MoRenderer.h"
#include "Window.h"
#include "model.h"
#include "Camera.h"
#include "Scene.h"


void HandleModelSkyboxSwitchEvents(Window* window, Scene* scene, MoRenderer* mo_renderer);

int main() {
	constexpr int width = 800;
	constexpr int height = 600;

	Window* window = Window::GetInstance();
	window->WindowInit(width, height, "MoRenderer");

#pragma region 外部资源加载

	const auto scene = new Scene();

	auto model = scene->current_model_;
	window->SetLogMessage("model_message", model->PrintModelInfo());
	window->SetLogMessage("model_name", "model name: " + scene->current_model_->model_name_);
	window->SetLogMessage("skybox_name", "skybox name: " + scene->current_iblmap_->skybox_name_);

#pragma endregion

#pragma region 配置UniformBuffer, 相机参数, 光源参数
	const Vec3f camera_position = { 0, 0, 2 };	// 相机位置
	const Vec3f camera_target = { 0, 0, 0 };	// 相机看向的位置
	const Vec3f camera_up = { 0, 1, 0 };		// 相机向上的位置
	constexpr float fov = 90.0f;				// 相加的垂直FOV
	auto* camera = new Camera(camera_position, camera_target, camera_up, fov, static_cast<float>(width) / height);

	const auto uniform_buffer = new UniformBuffer();
	uniform_buffer->model_matrix = model->model_matrix_;
	uniform_buffer->view_matrix = matrix_look_at(camera_position, camera_target, camera_up);
	uniform_buffer->proj_matrix = matrix_set_perspective(fov, camera->aspect_, camera->near_plane_, camera->near_plane_);
	uniform_buffer->CalculateRestMatrix();

	uniform_buffer->light_direction = { 0, -5, -2 };
	uniform_buffer->light_color = Vec3f(1.0f);
	uniform_buffer->camera_position = camera->position_;
#pragma endregion

#pragma region 配置Renderer
	const auto blinn_phong_shader = new BlinnPhongShader(uniform_buffer);
	const auto pbr_shader = new PBRShader(uniform_buffer);
	const auto skybox_shader = new SkyBoxShader(uniform_buffer);

	// 初始化渲染器
	const auto mo_renderer = new MoRenderer(width, height);
	mo_renderer->SetRenderState(false, true);
#pragma endregion

#pragma region RenderLoop

	while (!window->is_close_)
	{
		HandleModelSkyboxSwitchEvents(window, scene, mo_renderer);		// 切换天空盒和模型，切换线框渲染
		camera->HandleInputEvents();									// 更新相机参数
		scene->HandleKeyEvents(pbr_shader, blinn_phong_shader);			// 更新当前使用的shader

#pragma region 渲染Model
		model = scene->current_model_;
		switch (scene->current_shader_type_)
		{
		case kBlinnPhongShader:
			scene->UpdateShaderInfo(blinn_phong_shader);
			mo_renderer->SetVertexShader(blinn_phong_shader->vertex_shader_);
			mo_renderer->SetPixelShader(blinn_phong_shader->pixel_shader_);
			camera->UpdateUniformBuffer(blinn_phong_shader->uniform_buffer_, model->model_matrix_);

			blinn_phong_shader->HandleKeyEvents();
			break;
		case kPbrShader:
			scene->UpdateShaderInfo(pbr_shader);
			mo_renderer->SetVertexShader(pbr_shader->vertex_shader_);
			mo_renderer->SetPixelShader(pbr_shader->pixel_shader_);
			camera->UpdateUniformBuffer(pbr_shader->uniform_buffer_, model->model_matrix_);

			pbr_shader->HandleKeyEvents();
			break;
		default:;
		}

		mo_renderer->ClearFrameBuffer(mo_renderer->render_frame_, true);
		for (size_t i = 0; i < model->attributes_.size(); i += 3)
		{
			// 设置三个顶点的输入，供 VS 读取
			for (int j = 0; j < 3; j++) {
				switch (scene->current_shader_type_)
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
#pragma endregion


#pragma region 渲染Skybox
		scene->UpdateShaderInfo(skybox_shader);
		mo_renderer->SetVertexShader(skybox_shader->vertex_shader_);
		mo_renderer->SetPixelShader(skybox_shader->pixel_shader_);

		camera->UpdateSkyBoxUniformBuffer(skybox_shader->uniform_buffer_);
		camera->HandleInputEvents();
		camera->UpdateSkyboxMesh(skybox_shader);
		for (size_t i = 0; i < skybox_shader->plane_vertex_.size() - 2; i++)
		{
			skybox_shader->attributes_[0].position_os = skybox_shader->plane_vertex_[0];
			skybox_shader->attributes_[1].position_os = skybox_shader->plane_vertex_[i + 1];
			skybox_shader->attributes_[2].position_os = skybox_shader->plane_vertex_[i + 2];

			mo_renderer->DrawSkybox();
		}
#pragma endregion

		window->WindowDisplay(mo_renderer->color_buffer_);
	}


#pragma endregion

	return 0;
}


void HandleModelSkyboxSwitchEvents(Window* window, Scene* scene, MoRenderer* mo_renderer)
{
	if (window->can_press_keyboard_)
	{
		if (GetAsyncKeyState(VK_UP) & 0x8000)
		{
			scene->LoadPrevModel();
			window->SetLogMessage("model_message", scene->current_model_->PrintModelInfo());
			window->SetLogMessage("model_name", "model name: " + scene->current_model_->model_name_);
			window->can_press_keyboard_ = false;

		}
		else if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		{
			scene->LoadNextModel();
			window->SetLogMessage("model_message", scene->current_model_->PrintModelInfo());
			window->SetLogMessage("model_name", "model name: " + scene->current_model_->model_name_);
			window->can_press_keyboard_ = false;
		}
		else if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		{
			scene->LoadPrevIBLMap();
			window->SetLogMessage("skybox_name", "skybox name: " + scene->current_iblmap_->skybox_name_);
			window->can_press_keyboard_ = false;
		}
		else if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		{
			scene->LoadNextIBLMap();
			window->SetLogMessage("skybox_name", "skybox name: " + scene->current_iblmap_->skybox_name_);
			window->can_press_keyboard_ = false;
		}
		else if (window->keys_['0'])					// 切换渲染模式：线框渲染-像素渲染
		{
			if (mo_renderer->render_frame_)
			{
				mo_renderer->SetRenderState(false, true);
			}
			else
			{
				mo_renderer->SetRenderState(true, false);
			}
			window->can_press_keyboard_ = false;
		}

	}
}
