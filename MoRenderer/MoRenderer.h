#ifndef MORENDERER_H
#define MORENDERER_H

#include <map>
#include <functional>
#include <cstdint>


#include "math.h"
#include "model.h"
//---------------------------------------------------------------------
// 着色器定义
//---------------------------------------------------------------------

// 着色器上下文，由 VS 设置，再由渲染器按像素逐点插值后，供 PS 读取
struct ShaderContext {
	std::map<int, float> varying_float;    // 浮点数 varying 列表
	std::map<int, Vec2f> varying_vec2f;    // 二维矢量 varying 列表
	std::map<int, Vec3f> varying_vec3f;    // 三维矢量 varying 列表
	std::map<int, Vec4f> varying_vec4f;    // 四维矢量 varying 列表
};


// 顶点着色器：因为是 C++ 编写，无需传递 attribute，传个 0-2 的顶点序号
// 着色器函数直接在外层根据序号读取相应数据即可，最后需要返回一个坐标 pos
// 各项 varying 设置到 output 里，由渲染器插值后传递给 PS 
typedef std::function<Vec4f(int index, ShaderContext& output)> VertexShader;


// 像素着色器：输入 ShaderContext，需要返回 Vec4f 类型的颜色
// 三角形内每个点的 input 具体值会根据前面三个顶点的 output 插值得到
typedef std::function<Vec4f(ShaderContext& input)> PixelShader;


class MoRenderer
{
public:
	inline MoRenderer(int width, int height) {
		color_buffer = NULL;
		depth_buffer = NULL;
		render_frame = false;
		render_pixel = true;
		Init(width, height);
	}

	inline ~MoRenderer() { CleanUp(); }

public:
	// 初始化 frame buffer，渲染前需要先调用
	void Init(int width, int height);

	// 释放资源
	void CleanUp();

	// 清空 frame buffer
	void ClearFrameBuffer();

	// 设置 VS/PS 着色器函数
	inline void SetVertexShader(VertexShader vs) { vertex_shader = vs; }
	inline void SetPixelShader(PixelShader ps) { pixel_shader = ps; }

	// 设置背景/前景色
	inline void SetBGColor(Vec4f color) { color_background = color; }
	inline void SetFGColor(Vec4f color) { color_foreground = color; }

	// color buffer 里画点
	inline void SetPixel(int x, int y, const Vec4f& cc) { SetBuffer(color_buffer, x, y, cc); }
	inline void SetPixel(int x, int y, const Vec3f& cc) { SetBuffer(color_buffer, x, y, cc.xyz1()); }

	// color buffer 里画线
	inline void DrawLine(int x1, int y1, int x2, int y2) {
		if (color_buffer) DrawLine(x1, y1, x2, y2, color_foreground);
	}

	// 设置渲染状态，是否显示线框图，是否填充三角形
	inline void SetRenderState(bool frame, bool pixel) {
		render_frame = frame;
		render_pixel = pixel;
	}

	// 判断一条边是不是三角形的左上边 (Top-Left Edge)
	inline bool IsTopLeft(const Vec2i& a, const Vec2i& b) {
		return ((a.y == b.y) && (a.x < b.x)) || (a.y > b.y);
	}

	void SetBuffer(float** buffer, int x, int  y, float color) {
		assert(x < framebuffer_width);
		assert(y < framebuffer_height);
		buffer[x][y] = color;
	}

	void SetBuffer(Vec4f** buffer, int x, int  y, Vec4f color) {
		assert(x < framebuffer_width);
		assert(y < framebuffer_height);
		buffer[y][x] = color;
	}

public:

	//绘制一条线
	void DrawLine(int x1, int y1, int x2, int y2, Vec4f color);

	// 绘制一个三角形
	bool DrawTriangle();

	//绘制一个模型
	void DrawModel(Model* model);

protected:

	// 顶点结构体
	struct Vertex {
		ShaderContext context;    // 上下文
		float rhw;                // w 的倒数
		Vec4f pos;                // 坐标
		Vec2f spf;                // 浮点数屏幕坐标
		Vec2i spi;                // 整数屏幕坐标
	};

public:
	Vec4f** color_buffer;    // 颜色缓存
	float** depth_buffer;    // 深度缓存

	int framebuffer_width;            // frame buffer 宽度
	int framebuffer_height;           // frame buffer 高度
	Vec4f color_foreground;       // 前景色：画线时候用
	Vec4f color_background;       // 背景色：Clear 时候用

	Vertex _vertex[3];        // 三角形的三个顶点

	bool render_frame;       // 是否绘制线框
	bool render_pixel;       // 是否填充像素

	VertexShader vertex_shader;
	PixelShader pixel_shader;
};




#endif // !MORENDERER_H

