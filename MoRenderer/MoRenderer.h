#ifndef MO_RENDERER_H
#define MO_RENDERER_H

#include <functional>
#include <cstdint>
#include <ranges>

#include "math.h"
#include  "Shader.h"

class MoRenderer
{
public:

	MoRenderer() = default;

	MoRenderer(const int width, const int height) {
		color_buffer_ = nullptr;
		depth_buffer_ = nullptr;
		render_frame_ = false;
		render_pixel_ = true;
		Init(width, height);
	}

	~MoRenderer() { CleanUp(); }

public:
	// 初始化 frame buffer，渲染前需要先调用
	void Init(int width, int height);

	// 释放资源
	void CleanUp();

	// 清空 frame buffer
	void ClearFrameBuffer() const;

	// 设置 VS/PS 着色器函数
	void SetVertexShader(const VertexShader& vs) { vertex_shader_ = vs; }
	void SetPixelShader(const PixelShader& ps) { pixel_shader_ = ps; }


	// 设置背景/前景色
	void SetBackgroundColor(const Vec4f& color) { color_background_ = color; }
	void SetForegroundColor(const Vec4f& color) { color_foreground_ = color; }

	// 设置渲染状态，是否显示线框图，是否填充三角形
	void SetRenderState(const bool frame, const  bool pixel) {
		render_frame_ = frame;
		render_pixel_ = pixel;
	}

	static  void SetBuffer(float** buffer, const  int x, const  int  y, const float color) {
		buffer[x][y] = color;
	}

	void SetBuffer(uint8_t* buffer, const int x, const  int  y, const  Vec4f& color) const;


public:
	// 顶点结构体
	struct Vertex {
		bool has_transformed;			// 是否已经完成了顶点变换
		Varings context;			// 上下文
		float w_reciprocal;				// w 的倒数
		Vec4f position;					// 裁剪空间坐标	范围[-1,1]
		Vec2f screen_position_f;		// 屏幕坐标		范围x~[0.5, frame_buffer_width_+ 0.5] y~[0.5, frame_buffer_height_+ 0.5]
		Vec2i screen_position_i;		// 整数屏幕坐标  范围x~[0, frame_buffer_width_], y~[0, frame_buffer_height_]

		Vertex() {
			has_transformed = false;
		}

		~Vertex() = default;

	};

	static Vertex& VertexLerp(Vertex& vertex_p0, Vertex& vertex_p1, float ratio);

	// 边缘方程e(x, y)（详见RTR4 章节23.1）
	struct EdgeEquation
	{
		float a, b, c;		// 边缘方程的系数
		bool is_top_left;	// 这条边缘是否属于左边缘或者上边缘	

		float origin;		// 边缘左下角的起始值

		float w_reciprocal;	// 对顶点w分量的倒数

		void Initialize(const Vec2f& p0, const Vec2f& p1, const Vec2f& bottom_left_point, float w_reciprocal)
		{
			// 详见RTR4 方程23.2
			a = -(p1.y - p0.y);
			b = p1.x - p0.x;
			c = -a * p0.x - b * p0.y;

			origin = a * bottom_left_point.x + b * bottom_left_point.y + c;

			/*
			 * 当(x, y)位于三角形内部和边缘上时，有e>=0
			 *
			 * 当边或者顶点恰好经过像素中心时，使用top-left规则来判断像素归属
			 * 当像素中心位于上边缘或者和左边缘的时候，认为这个像素位于三角形内部
			 * 上边缘：边缘方程中的a=0, b<0
			 * 左边缘：边缘方程中的a>0
			 *
			 */
			is_top_left = (NearEqual(a, 0.0f, 0.00001f) && b > 0) || a > 0;

			this->w_reciprocal = w_reciprocal;
		}

		float Evaluate(const int x, const int y) const
		{
			return origin + x * a + y * b;
		}
	};

	// 裁剪空间下的裁剪平面
	enum ClipPlane
	{
		W_Plane,
		X_RIGHT,
		X_LEFT,
		Y_TOP,
		Y_BOTTOM,
		Z_Near,
		Z_FAR
	};

public:
	// color buffer 里画线段
	void DrawLine(const int x1, const int y1, const int x2, const int y2) const {
		if (color_buffer_) DrawLine(x1, y1, x2, y2, color_foreground_);
	}

	// color buffer 里画点
	void SetPixel(const int x, const int y, const Vec4f& cc) const { SetBuffer(color_buffer_, x, y, cc); }
	void SetPixel(const int x, const int y, const Vec3f& cc)const { SetBuffer(color_buffer_, x, y, cc.xyz1()); }

	int ClipWithPlane(ClipPlane clip_plane, Vertex vertex[3]);

	// 绘制三角形
	void DrawSkybox();

	// 绘制三角形
	void DrawMesh();
	// 光栅化三角形
	void RasterizeTriangle(Vertex vertex[3]);

	// 绘制线框
	void DrawWireFrame(Vertex vertex[3]) const;
	// 绘制一条线
	void DrawLine(int x1, int y1, int x2, int y2, const Vec4f& color) const;

public:
	uint8_t* color_buffer_;			// 颜色缓存
	float** depth_buffer_;			// 深度缓存

	int frame_buffer_width_;		// frame buffer 宽度
	int frame_buffer_height_;		// frame buffer 高度
	Vec4f color_foreground_;		// 前景色：画线时候用
	Vec4f color_background_;		// 背景色：Clear 时候用

	bool render_frame_;				// 是否绘制线框
	bool render_pixel_;				// 是否填充像素

	// 渲染中使用的临时数据
	Vertex vertex_[3];				// 三角形的顶点
	Vertex clip_vertex_[4];			// 三角形的顶点
	EdgeEquation edge_equation_[3];
	Varings current_varings_;

	VertexShader vertex_shader_;
	PixelShader pixel_shader_;


};




#endif	

