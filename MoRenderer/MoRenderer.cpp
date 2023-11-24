
#include "MoRenderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION	// include之前必须定义
#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>

int main()
{
	const int width = 800;
	const int height = 600;


	// 初始化渲染器和帧缓存大小
	MoRenderer* rh = new MoRenderer(800, 600);

	const int VARYING_COLOR = 0;    // 定义一个 varying 的 key

	// 顶点数据，由 VS 读取，如有多个三角形，可每次更新 vs_input 再绘制
	struct { Vec4f pos; Vec4f color; } vs_input[3] = {
		{ {  0.0,  0.7, 0.90, 1}, {1, 0, 0, 1} },
		{ { -0.6, -0.2, 0.01, 1}, {0, 1, 0, 1} },
		{ { +0.6, -0.2, 0.01, 1}, {0, 0, 1, 1} },
	};

	// 顶点着色器，初始化 varying 并返回坐标，
	// 参数 index 是渲染器传入的顶点序号，范围 [0, 2] 用于读取顶点数据
	rh->SetVertexShader([&](int index, ShaderContext& output) -> Vec4f {
		output.varying_vec4f[VARYING_COLOR] = vs_input[index].color;
		return vs_input[index].pos;		// 直接返回坐标
		});

	// 像素着色器，返回颜色
	rh->SetPixelShader([&](ShaderContext& input) -> Vec4f {
		return input.varying_vec4f[VARYING_COLOR];
		});

	// 渲染并保存
	rh->DrawPrimitive();


	uint8_t* image_data = new uint8_t[width * height * 4];
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			Vec4i color = vector_to_color_Vec4i(rh->_color_buffer[y][x]);

			image_data[4 * (y * width + x)] = color[0];
			image_data[4 * (y * width + x) + 1] = color[1];
			image_data[4 * (y * width + x) + 2] = color[2];
			image_data[4 * (y * width + x) + 3] = color[3];
		}
	}

	stbi_write_png("C:/WorkSpace/MoRenderer/out.png", width, height, 4, image_data, width * 4);

	return 0;
}

