#include <iostream>
#include <fstream>

#include "MoRenderer.h"
#include "win32.h"
#include "model.h"
#include "Texture.h"

int main() {

	const int width = 600;
	const int height = 800;

	window_init(width, height, "MoRenderer");

	int num_frames = 0;
	float print_time = platform_get_time();
	std::string logMessage;


	MoRenderer* rh = new MoRenderer(width, height);

	// ����ģ��
	const std::string modelName = "C:/WorkSpace/MoRenderer/models/diablo3_pose.obj";
	Model* model = new Model(modelName);

	// ������ͼ
	Texture* diffuse = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_diffuse.bmp");
	Texture* normal = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_nm.bmp");
	Texture* specular = new Texture("C:/WorkSpace/MoRenderer/models/diablo3_pose_spec.bmp"); 
	 
	// ��������͹�Դ
	Vec3f eye_pos = { 0, -0.5, 1.7 };			// ���λ��
	Vec3f eye_at = { 0, 0, 0 };					// ��������λ��
	Vec3f eye_up = { 0, 1, 0 };					// ������ϵ�λ��
	Vec3f light_dir = { 1, 1, 0.85 };			// ���շ���
	float perspective = 3.1415926f * 0.5f;	

	// ���ñ任����
	Mat4x4f mat_model = matrix_set_scale(1, 1, 1);
	Mat4x4f mat_view = matrix_set_lookat(eye_pos, eye_at, eye_up);
	Mat4x4f mat_proj = matrix_set_perspective(perspective, 6 / 8.0, 1.0, 500.0f);
	Mat4x4f mat_mvp = mat_model * mat_view * mat_proj;

	// ���ڽ����ߴ�ģ������ϵ�任����������ϵ
	Mat4x4f mat_model_it = matrix_invert(mat_model).Transpose();

	// ��������
	Vertex vs_input[3];
	enum VARYING_ATTRIBUTES
	{
		VARYING_TEXCOORD = 0,		// ��������
		VARYING_POSITIONWS = 1		// ��������
	};

	// ������ɫ��
	rh->SetVertexShader([&](int index, ShaderContext& output) -> Vec4f {
		Vec4f vertex = vs_input[index].positionOS.xyz1() * mat_mvp;
		// ������λ�ô�ģ�Ϳռ�ת��Ϊ��������ϵ
		Vec3f positionWS = (vs_input[index].positionOS.xyz1() * mat_model).xyz();
		output.varying_vec3f[VARYING_POSITIONWS] = positionWS;
		output.varying_vec2f[VARYING_TEXCOORD] = vs_input[index].texcoord;
		return vertex;
		});


	// ������ɫ����ʹ��Blinn-Phong����ģ��
	rh->SetPixelShader([&](ShaderContext& input) -> Vec4f {
		Vec2f uv = input.varying_vec2f[VARYING_TEXCOORD];

		Vec3f worldNormal = (normal->Sample2D(uv) * mat_model_it).xyz();
		Vec3f worldPosition = input.varying_vec3f[VARYING_TEXCOORD];

		Vec3f lightDir = vector_normalize(light_dir);
		Vec3f viewDir = vector_normalize(lightDir - worldPosition);

		//������
		Vec4f baseColor = diffuse->Sample2D(uv);
		Vec4f diffuse = baseColor * Saturate(vector_dot(lightDir, worldNormal));

		//�߹�
		float _Specluar = specular->Sample2D(uv).b * 5;
		Vec3f halfDir = vector_normalize(viewDir + lightDir);
		float intensity = pow(Saturate(vector_dot(worldNormal, halfDir)), 20);
		Vec4f specular = baseColor * intensity * _Specluar;

		Vec4f color = diffuse + specular;
		return color;
		});

	while (!window->is_close)
	{
		float curr_time = platform_get_time();

		// ��Ⱦģ��
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
		unsigned char* image_data = new uint8_t[width * height * 4];
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				ColorRGBA_32bit color = vector_to_32bit_color(rh->color_buffer[y][x]);

				//32 bitλͼ�洢˳�򣬴ӵ͵�������ΪBGRA
				image_data[4 * (y * width + x)] = color.b;
				image_data[4 * (y * width + x) + 1] = color.g;
				image_data[4 * (y * width + x) + 2] = color.r;
				image_data[4 * (y * width + x) + 3] = color.a;
			}
		}

		// ���㲢��ʾFPS
		num_frames += 1;
		if (curr_time - print_time >= 1) {
			int sum_millis = (int)((curr_time - print_time) * 1000);
			int avg_millis = sum_millis / num_frames;

			logMessage = "FPS: " + std::to_string(num_frames) + " / " + std::to_string(avg_millis) + " ms";

			num_frames = 0;
			print_time = curr_time;
		}

		window_draw(image_data, logMessage);
		msg_dispatch();
	}

	return 0;
}
