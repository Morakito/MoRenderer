#ifndef MORENDERER_H
#define MORENDERER_H


#include "math.h"
#include <map>
#include <functional>
#include <cstdint>
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
	inline virtual ~MoRenderer() { Reset(); }

	inline MoRenderer() {
		_color_buffer = NULL;
		_depth_buffer = NULL;
		_render_frame = false;
		_render_pixel = true;
	}

	inline MoRenderer(int width, int height) {
		_color_buffer = NULL;
		_depth_buffer = NULL;
		_render_frame = false;
		_render_pixel = true;
		Init(width, height);
	}

public:
	// 复位状态
	inline void Reset() {
		_vertex_shader = NULL;
		_pixel_shader = NULL;

		if (_color_buffer) {
			for (int j = 0; j < _framebuffer_height; j++) {
				if (_color_buffer[j]) delete[]_color_buffer[j];
				_color_buffer[j] = NULL;
			}
			delete[]_color_buffer;
			_color_buffer = NULL;
		}

		if (_depth_buffer) {
			for (int j = 0; j < _framebuffer_height; j++) {
				if (_depth_buffer[j]) delete[]_depth_buffer[j];
				_depth_buffer[j] = NULL;
			}
			delete[]_depth_buffer;
			_depth_buffer = NULL;
		}

		_color_fg = Vec4f(1.0f, 1.0f, 1.0f, 1.0f);
		_color_bg = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
	}

	// 初始化 FrameBuffer，渲染前需要先调用
	inline void Init(int width, int height) {
		Reset();

		_framebuffer_width = width;
		_framebuffer_height = height;

		_color_buffer = new Vec4f * [height];
		for (int j = 0; j < height; j++) {
			_color_buffer[j] = new Vec4f[width];
		}

		_depth_buffer = new float* [height];
		for (int j = 0; j < height; j++) {
			_depth_buffer[j] = new float[width];
		}
		Clear();
	}

	// 清空 FrameBuffer 和深度缓存
	inline void Clear() {
		if (_color_buffer) {
			for (int j = 0; j < _framebuffer_height; j++) {
				for (int i = 0; i < _framebuffer_width; i++)
					_color_buffer[j][i] = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
			}
		}
		if (_depth_buffer) {
			for (int j = 0; j < _framebuffer_height; j++) {
				for (int i = 0; i < _framebuffer_width; i++)
					_depth_buffer[j][i] = 0.0f;
			}
		}
	}

	// 设置 VS/PS 着色器函数
	inline void SetVertexShader(VertexShader vs) { _vertex_shader = vs; }
	inline void SetPixelShader(PixelShader ps) { _pixel_shader = ps; }

	// 设置背景/前景色
	inline void SetBGColor(Vec4f color) { _color_bg = color; }
	inline void SetFGColor(Vec4f color) { _color_fg = color; }

	// color buffer 里画点
	inline void SetPixel(int x, int y, const Vec4f& cc) { SetBuffer(_color_buffer, x, y, cc); }
	inline void SetPixel(int x, int y, const Vec3f& cc) { SetBuffer(_color_buffer, x, y, cc.xyz1()); }

	// color buffer 里画线
	inline void DrawLine(int x1, int y1, int x2, int y2) {
		if (_color_buffer) DrawLine(x1, y1, x2, y2, _color_fg);
	}

	inline void DrawLine(int x1, int y1, int x2, int y2, Vec4f color) {
		int x, y;
		if (x1 == x2 && y1 == y2) {
			SetPixel(x1, y1, color);
			return;
		}
		else if (x1 == x2) {
			int inc = (y1 <= y2) ? 1 : -1;
			for (y = y1; y != y2; y += inc) SetPixel(x1, y, color);
			SetPixel(x2, y2, color);
		}
		else if (y1 == y2) {
			int inc = (x1 <= x2) ? 1 : -1;
			for (x = x1; x != x2; x += inc) SetPixel(x, y1, color);
			SetPixel(x2, y2, color);
		}
		else {
			int dx = (x1 < x2) ? x2 - x1 : x1 - x2;
			int dy = (y1 < y2) ? y2 - y1 : y1 - y2;
			int rem = 0;
			if (dx >= dy) {
				if (x2 < x1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
				for (x = x1, y = y1; x <= x2; x++) {
					SetPixel(x, y, color);
					rem += dy;
					if (rem >= dx) { rem -= dx; y += (y2 >= y1) ? 1 : -1; SetPixel(x, y, color); }
				}
				SetPixel(x2, y2, color);
			}
			else {
				if (y2 < y1) x = x1, y = y1, x1 = x2, y1 = y2, x2 = x, y2 = y;
				for (x = x1, y = y1; y <= y2; y++) {
					SetPixel(x, y, color);
					rem += dx;
					if (rem >= dy) { rem -= dy; x += (x2 >= x1) ? 1 : -1; SetPixel(x, y, color); }
				}
				SetPixel(x2, y2, color);
			}
		}
	}

	// 设置渲染状态，是否显示线框图，是否填充三角形
	inline void SetRenderState(bool frame, bool pixel) {
		_render_frame = frame;
		_render_pixel = pixel;
	}

	// 判断一条边是不是三角形的左上边 (Top-Left Edge)
	inline bool IsTopLeft(const Vec2i& a, const Vec2i& b) {
		return ((a.y == b.y) && (a.x < b.x)) || (a.y > b.y);
	}


	void SetBuffer(float** buffer, int x, int  y, float color) {
		buffer[x][y] = color;
	}

	void SetBuffer(Vec4f** buffer, int x, int  y, Vec4f color) {
		assert(x < _framebuffer_width);
		assert(y < _framebuffer_height);
		buffer[y][x] = color;
	}

	void Log(std::string message) {
		std::cout << message << std::endl;
	}

public:

	// 绘制一个三角形，必须先设定好着色器函数
	inline bool DrawPrimitive() {
		if (_color_buffer == NULL || _vertex_shader == NULL)
			return false;

		// 顶点初始化
		for (int k = 0; k < 3; k++) {
			Vertex& vertex = _vertex[k];

			// 清空上下文 varying 列表
			vertex.context.varying_float.clear();
			vertex.context.varying_vec2f.clear();
			vertex.context.varying_vec3f.clear();
			vertex.context.varying_vec4f.clear();

			// 运行顶点着色程序，返回顶点坐标
			vertex.pos = _vertex_shader(k, vertex.context);

			// 简单裁剪，任何一个顶点超过 CVV 就剔除
			float w = vertex.pos.w;

			// 这里图简单，当一个点越界，立马放弃整个三角形，更精细的做法是
			// 如果越界了就在齐次空间内进行裁剪，拆分为 0-2 个三角形然后继续
			if (w == 0.0f) return false;
			if (vertex.pos.z < 0.0f || vertex.pos.z > w) return false;
			if (vertex.pos.x < -w || vertex.pos.x > w) return false;
			if (vertex.pos.y < -w || vertex.pos.y > w) return false;

			// 计算 w 的倒数：Reciprocal of the Homogeneous W 
			vertex.rhw = 1.0f / w;

			// 齐次坐标空间 /w 归一化到单位体积 cvv
			vertex.pos *= vertex.rhw;

			// 计算屏幕坐标
			vertex.spf.x = (vertex.pos.x + 1.0f) * _framebuffer_width * 0.5f;
			vertex.spf.y = (1.0f - vertex.pos.y) * _framebuffer_height * 0.5f;

			// 整数屏幕坐标：加 0.5 的偏移取屏幕像素方格中心对齐
			vertex.spi.x = (int)(vertex.spf.x + 0.5f);
			vertex.spi.y = (int)(vertex.spf.y + 0.5f);

			// 更新外接矩形范围
			if (k == 0) {
				_min_x = _max_x = Between(0, _framebuffer_width - 1, vertex.spi.x);
				_min_y = _max_y = Between(0, _framebuffer_height - 1, vertex.spi.y);
			}
			else {
				_min_x = Between(0, _framebuffer_width - 1, Min(_min_x, vertex.spi.x));
				_max_x = Between(0, _framebuffer_width - 1, Max(_max_x, vertex.spi.x));
				_min_y = Between(0, _framebuffer_height - 1, Min(_min_y, vertex.spi.y));
				_max_y = Between(0, _framebuffer_height - 1, Max(_max_y, vertex.spi.y));
			}
		}

		// 绘制线框
		if (_render_frame) {
			DrawLine(_vertex[0].spi.x, _vertex[0].spi.y, _vertex[1].spi.x, _vertex[1].spi.y);
			DrawLine(_vertex[0].spi.x, _vertex[0].spi.y, _vertex[2].spi.x, _vertex[2].spi.y);
			DrawLine(_vertex[2].spi.x, _vertex[2].spi.y, _vertex[1].spi.x, _vertex[1].spi.y);
		}

		// 如果不填充像素就退出
		if (_render_pixel == false) return false;

		// 判断三角形朝向
		Vec4f v01 = _vertex[1].pos - _vertex[0].pos;
		Vec4f v02 = _vertex[2].pos - _vertex[0].pos;
		Vec4f normal = vector_cross(v01, v02);

		// 使用 vtx 访问三个顶点，而不直接用 _vertex 访问，因为可能会调整顺序
		Vertex* vtx[3] = { &_vertex[0], &_vertex[1], &_vertex[2] };

		// 如果背向视点，则交换顶点，保证 edge equation 判断的符号为正
		if (normal.z > 0.0f) {
			vtx[1] = &_vertex[2];
			vtx[2] = &_vertex[1];
		}
		else if (normal.z == 0.0f) {
			return false;
		}

		// 保存三个端点位置
		Vec2i p0 = vtx[0]->spi;
		Vec2i p1 = vtx[1]->spi;
		Vec2i p2 = vtx[2]->spi;

		// 计算面积，为零就退出
		float s = Abs(vector_cross(p1 - p0, p2 - p0));
		if (s <= 0) return false;

		// 三角形填充时，左面和上面的边上的点需要包括，右方和下方边上的点不包括
		// 先判断是否是 TopLeft，判断出来后会和下方 Edge Equation 一起决策
		bool TopLeft01 = IsTopLeft(p0, p1);
		bool TopLeft12 = IsTopLeft(p1, p2);
		bool TopLeft20 = IsTopLeft(p2, p0);

		// 迭代三角形外接矩形的所有点
		for (int cy = _min_y; cy <= _max_y; cy++) {
			for (int cx = _min_x; cx <= _max_x; cx++) {
				Vec2f px = { (float)cx + 0.5f, (float)cy + 0.5f };

				// Edge Equation
				// 使用整数避免浮点误差，同时因为是左手系，所以符号取反
				int E01 = -(cx - p0.x) * (p1.y - p0.y) + (cy - p0.y) * (p1.x - p0.x);
				int E12 = -(cx - p1.x) * (p2.y - p1.y) + (cy - p1.y) * (p2.x - p1.x);
				int E20 = -(cx - p2.x) * (p0.y - p2.y) + (cy - p2.y) * (p0.x - p2.x);


				// 如果是左上边，用 E >= 0 判断合法，如果右下边就用 E > 0 判断合法
				// 这里通过引入一个误差 1 ，来将 < 0 和 <= 0 用一个式子表达
				if (E01 < (TopLeft01 ? 0 : 1)) continue;   // 在第一条边后面
				if (E12 < (TopLeft12 ? 0 : 1)) continue;   // 在第二条边后面
				if (E20 < (TopLeft20 ? 0 : 1)) continue;   // 在第三条边后面

				// 三个端点到当前点的矢量
				Vec2f s0 = vtx[0]->spf - px;
				Vec2f s1 = vtx[1]->spf - px;
				Vec2f s2 = vtx[2]->spf - px;

				// 重心坐标系：计算内部子三角形面积 a / b / c
				float a = Abs(vector_cross(s1, s2));    // 子三角形 Px-P1-P2 面积
				float b = Abs(vector_cross(s2, s0));    // 子三角形 Px-P2-P0 面积
				float c = Abs(vector_cross(s0, s1));    // 子三角形 Px-P0-P1 面积
				float s = a + b + c;                    // 大三角形 P0-P1-P2 面积

				if (s == 0.0f) continue;

				// 除以总面积，以保证：a + b + c = 1，方便用作插值系数
				a = a * (1.0f / s);
				b = b * (1.0f / s);
				c = c * (1.0f / s);

				// 计算当前点的 1/w，因 1/w 和屏幕空间呈线性关系，故直接重心插值
				float rhw = vtx[0]->rhw * a + vtx[1]->rhw * b + vtx[2]->rhw * c;

				// 进行深度测试
				if (rhw < _depth_buffer[cy][cx]) continue;
				_depth_buffer[cy][cx] = rhw;   // 记录 1/w 到深度缓存

				// 还原当前像素的 w
				float w = 1.0f / ((rhw != 0.0f) ? rhw : 1.0f);

				// 计算三个顶点插值 varying 的系数
				// 先除以各自顶点的 w 然后进行屏幕空间插值然后再乘以当前 w
				float c0 = vtx[0]->rhw * a * w;
				float c1 = vtx[1]->rhw * b * w;
				float c2 = vtx[2]->rhw * c * w;

				// 准备为当前像素的各项 varying 进行插值
				ShaderContext input;

				ShaderContext& i0 = vtx[0]->context;
				ShaderContext& i1 = vtx[1]->context;
				ShaderContext& i2 = vtx[2]->context;

				// 插值各项 varying
				for (auto const& it : i0.varying_float) {
					int key = it.first;
					float f0 = i0.varying_float[key];
					float f1 = i1.varying_float[key];
					float f2 = i2.varying_float[key];
					input.varying_float[key] = c0 * f0 + c1 * f1 + c2 * f2;
				}

				for (auto const& it : i0.varying_vec2f) {
					int key = it.first;
					const Vec2f& f0 = i0.varying_vec2f[key];
					const Vec2f& f1 = i1.varying_vec2f[key];
					const Vec2f& f2 = i2.varying_vec2f[key];
					input.varying_vec2f[key] = c0 * f0 + c1 * f1 + c2 * f2;
				}

				for (auto const& it : i0.varying_vec3f) {
					int key = it.first;
					const Vec3f& f0 = i0.varying_vec3f[key];
					const Vec3f& f1 = i1.varying_vec3f[key];
					const Vec3f& f2 = i2.varying_vec3f[key];
					input.varying_vec3f[key] = c0 * f0 + c1 * f1 + c2 * f2;
				}

				for (auto const& it : i0.varying_vec4f) {
					int key = it.first;
					const Vec4f& f0 = i0.varying_vec4f[key];
					const Vec4f& f1 = i1.varying_vec4f[key];
					const Vec4f& f2 = i2.varying_vec4f[key];
					input.varying_vec4f[key] = c0 * f0 + c1 * f1 + c2 * f2;
				}

				// 执行像素着色器
				Vec4f color = { 0.0f, 0.0f, 0.0f, 0.0f };

				if (_pixel_shader != NULL) {
					color = _pixel_shader(input);
				}

				// 绘制到 framebuffer 上，这里可以加判断，如果 PS 返回的颜色 alpha 分量
				// 小于等于零则放弃绘制，不过这样的话要把前面的更新深度缓存的代码挪下来，
				// 只有需要渲染的时候才更新深度。
				SetPixel(cx, cy, color);
			}
		}

		// 绘制线框，再画一次避免覆盖
		if (_render_frame) {
			DrawLine(_vertex[0].spi.x, _vertex[0].spi.y, _vertex[1].spi.x, _vertex[1].spi.y);
			DrawLine(_vertex[0].spi.x, _vertex[0].spi.y, _vertex[2].spi.x, _vertex[2].spi.y);
			DrawLine(_vertex[2].spi.x, _vertex[2].spi.y, _vertex[1].spi.x, _vertex[1].spi.y);
		}

		return true;
	}

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
	Vec4f** _color_buffer;    // 颜色缓存
	float** _depth_buffer;    // 深度缓存

	int _framebuffer_width;            // frame buffer 宽度
	int _framebuffer_height;           // frame buffer 高度
	Vec4f _color_fg;       // 前景色：画线时候用
	Vec4f _color_bg;       // 背景色：Clear 时候用

	Vertex _vertex[3];        // 三角形的三个顶点

	int _min_x;               // 三角形外接矩形
	int _min_y;
	int _max_x;
	int _max_y;

	bool _render_frame;       // 是否绘制线框
	bool _render_pixel;       // 是否填充像素

	VertexShader _vertex_shader;
	PixelShader _pixel_shader;

};




#endif // !MORENDERER_H

