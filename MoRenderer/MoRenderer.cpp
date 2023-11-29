#include "MoRenderer.h"

#include <ranges>

void MoRenderer::CleanUp()
{
	// 清空着色器
	vertex_shader_ = nullptr;
	pixel_shader_ = nullptr;

	// 清空frame buffer
	if (color_buffer_) {
		delete[]color_buffer_;
		color_buffer_ = nullptr;
	}

	if (depth_buffer_) {
		for (int j = 0; j < frame_buffer_height_; j++) {
			if (depth_buffer_[j]) delete[]depth_buffer_[j];
			depth_buffer_[j] = nullptr;
		}
		delete[]depth_buffer_;
		depth_buffer_ = nullptr;
	}
}

void MoRenderer::Init(const int width, const int height)
{

	frame_buffer_width_ = width;
	frame_buffer_height_ = height;

	// 初始化背景色和前景色
	color_foreground_ = Vec4f(0.0f);
	color_background_ = Vec4f(0.5f, 1.0f, 1.0f, 1.0f);

	color_buffer_ = new uint8_t[height * width * 4];

	depth_buffer_ = new float* [height];
	for (int j = 0; j < height; j++) {
		depth_buffer_[j] = new float[width];
	}

	ClearFrameBuffer();
}

void MoRenderer::ClearFrameBuffer() const
{
	const ColorRGBA_32bit color_32_bit = vector_to_32bit_color(color_background_);
	if (color_buffer_) {
		for (int j = 0; j < frame_buffer_height_; j++) {
			const int offset = frame_buffer_width_ * (4 * j);

			for (int i = 0; i < frame_buffer_width_; i++)
			{
				const int base_address = offset + 4 * i;
				//32 bit位图存储顺序，从低到高依次为BGRA
				color_buffer_[base_address] = color_32_bit.b;
				color_buffer_[base_address + 1] = color_32_bit.g;
				color_buffer_[base_address + 2] = color_32_bit.r;
				color_buffer_[base_address + 3] = color_32_bit.a;
			}
		}
	}

	if (depth_buffer_) {
		for (int j = 0; j < frame_buffer_height_; j++) {
			for (int i = 0; i < frame_buffer_width_; i++)
				depth_buffer_[j][i] = 0.0f;
		}
	}
}

void MoRenderer::SetBuffer(uint8_t* buffer, const int x, const int y, const Vec4f& color) const
{
	const ColorRGBA_32bit color_32_bit = vector_to_32bit_color(color);
	const int base_address = frame_buffer_width_ * (4 * y) + 4 * x;
	//32 bit位图存储顺序，从低到高依次为BGRA
	buffer[base_address] = color_32_bit.b;
	buffer[base_address + 1] = color_32_bit.g;
	buffer[base_address + 2] = color_32_bit.r;
	buffer[base_address + 3] = color_32_bit.a;
}

void MoRenderer::DrawLine(int x1, int y1, int x2, int y2, const Vec4f& color) const
{
	int x, y;
	if (x1 == x2 && y1 == y2) {
		SetPixel(x1, y1, color);
		return;
	}
	else if (x1 == x2) {
		const int dir = (y1 <= y2) ? 1 : -1;
		for (y = y1; y != y2; y += dir) SetPixel(x1, y, color);
		SetPixel(x2, y2, color);
	}
	else if (y1 == y2) {
		const int dir = (x1 <= x2) ? 1 : -1;
		for (x = x1; x != x2; x += dir) SetPixel(x, y1, color);
		SetPixel(x2, y2, color);
	}
	else {
		// 选择绘制的主轴，沿着跨度较大的轴进行绘制
		const int dx = (x1 < x2) ? x2 - x1 : x1 - x2;
		const int dy = (y1 < y2) ? y2 - y1 : y1 - y2;
		int rem = 0;
		if (dx >= dy) {
			// 交换(x1, y1)和(x1, y1)，使得x1较小
			if (x2 < x1) {
				std::swap(x1, x2);
				std::swap(y1, y2);
				x = x1; y = y1;
			}
			for (x = x1, y = y1; x <= x2; x++) {
				SetPixel(x, y, color);
				rem += dy;
				if (rem >= dx) {
					rem -= dx;
					y += (y2 >= y1) ? 1 : -1; SetPixel(x, y, color);
				}
			}
			SetPixel(x2, y2, color);
		}
		else {
			// 交换(x1, y1)和(x1, y1)，使得y1较小
			if (y2 < y1) {
				std::swap(x1, x2);
				std::swap(y1, y2);
				x = x1; y = y1;
			}
			for (x = x1, y = y1; y <= y2; y++) {
				SetPixel(x, y, color);
				rem += dx;
				if (rem >= dy) {
					rem -= dy;
					x += (x2 >= x1) ? 1 : -1; SetPixel(x, y, color);
				}
			}
			SetPixel(x2, y2, color);
		}
	}
}

bool MoRenderer::DrawTriangle() {
	if (color_buffer_ == nullptr || vertex_shader_ == nullptr) return false;

	// 三角形屏幕空间中的外接矩形
	Vec2i bounding_min(0), bounding_max(0);

	Vertex* vertex[3] = { &vertex_[0], &vertex_[1], &vertex_[2] };
	// 顶点数据初始化
	for (int k = 0; k < 3; k++) {
		auto& [context, w_reciprocal, position, screen_position_f, screen_position_i] = *vertex[k];

		// 清空上下文 varying 列表
		context.varying_float.clear();
		context.varying_vec2f.clear();
		context.varying_vec3f.clear();
		context.varying_vec4f.clear();

		// 执行顶点着色程序，返回裁剪空间中的顶点坐标，此时没有进行透视除法
		position = vertex_shader_(k, context);

		// 裁剪clip：三角形的任何一个顶点超过CVV就直接剔除
		float w = position.w;
		if (w == 0.0f) return false;
		if (position.z < -w || position.z > w) return false;
		if (position.x < -w || position.x > w) return false;
		if (position.y < -w || position.y > w) return false;

		// 透视除法
		w_reciprocal = 1.0f / w;
		position *= w_reciprocal;

		// 屏幕映射：计算屏幕坐标（窗口坐标。详见RTR4 章节2.3.4
		screen_position_f.x = (position.x + 1.0f) * static_cast<float>(frame_buffer_width_) * 0.5f;
		screen_position_f.y = (position.y + 1.0f) * static_cast<float>(frame_buffer_height_) * 0.5f;

		// 计算整数屏幕坐标：d = floor(c)
		screen_position_i.x = static_cast<int>(floor(screen_position_f.x));
		screen_position_i.y = static_cast<int>(floor(screen_position_f.y));

		//计算整数屏幕坐标：c = d + 0.5
		screen_position_f.x = screen_position_i.x + 0.5f;
		screen_position_f.y = screen_position_i.y + 0.5f;

		// 更新外接矩形
		if (k == 0) {
			bounding_min.x = bounding_max.x = Between(0, frame_buffer_width_ - 1, screen_position_i.x);
			bounding_min.y = bounding_max.y = Between(0, frame_buffer_height_ - 1, screen_position_i.y);
		}
		else {
			bounding_min.x = Between(0, frame_buffer_width_ - 1, Min(bounding_min.x, screen_position_i.x));
			bounding_max.x = Between(0, frame_buffer_width_ - 1, Max(bounding_max.x, screen_position_i.x));
			bounding_min.y = Between(0, frame_buffer_height_ - 1, Min(bounding_min.y, screen_position_i.y));
			bounding_max.y = Between(0, frame_buffer_height_ - 1, Max(bounding_max.y, screen_position_i.y));
		}
	}

	// 只绘制线框，不绘制像素，直接退出
	if (render_frame_ && !render_pixel_) {
		DrawLine(vertex[0]->screen_position_i.x, vertex[0]->screen_position_i.y, vertex[1]->screen_position_i.x, vertex[1]->screen_position_i.y);
		DrawLine(vertex[0]->screen_position_i.x, vertex[0]->screen_position_i.y, vertex[2]->screen_position_i.x, vertex[2]->screen_position_i.y);
		DrawLine(vertex[2]->screen_position_i.x, vertex[2]->screen_position_i.y, vertex[1]->screen_position_i.x, vertex[1]->screen_position_i.y);

		return false;
	}

	/*
	* 裁剪空间，背面剔除：
	*
	* 在观察空间中进行判断，观察空间使用右手坐标系，即相机看向z轴负方向
	* 判断三角形朝向，剔除背对相机的三角形
	* 由于相机看向z轴负方向，因此三角形法线的z分量为负，说明背对相机
	*
	* 顶点顺序：
	* obj格式中默认的顶点顺序是逆时针，即顶点v1，v2，v3按照逆时针顺序排列
	*/
	Vec4f vector_01 = vertex[1]->position - vertex[0]->position;
	Vec4f vector_02 = vertex[2]->position - vertex[0]->position;
	Vec4f normal = vector_cross(vector_01, vector_02);
	if (normal.z <= 0) return false;

	// 保存三个端点位置
	Vec2f p0 = vertex[0]->screen_position_f;
	Vec2f p1 = vertex[1]->screen_position_f;
	Vec2f p2 = vertex[2]->screen_position_f;

	// 屏幕空间，退化三角形剔除
	float triangle_area = Abs(vector_cross(p1 - p0, p2 - p0));
	if (triangle_area <= 0) return false;

	// 构建边缘方程
	Vec2f bottom_left_point = { static_cast<float>(bounding_min.x) + 0.5f,static_cast<float>(bounding_min.y) + 0.5f };
	auto* edge_equation_01 = new EdgeEquation(p0, p1, bottom_left_point, vertex[2]->w_reciprocal);
	auto* edge_equation_12 = new EdgeEquation(p1, p2, bottom_left_point, vertex[0]->w_reciprocal);
	auto* edge_equation_20 = new EdgeEquation(p2, p0, bottom_left_point, vertex[1]->w_reciprocal);

	float bc_denominator =
		edge_equation_01->origin * edge_equation_01->w_reciprocal +
		edge_equation_12->origin * edge_equation_12->w_reciprocal +
		edge_equation_20->origin * edge_equation_20->w_reciprocal;
	bc_denominator = 1 / bc_denominator;

	// 迭代三角形外接矩形的所有点
	for (int y = bounding_min.y; y <= bounding_max.y; y++) {
		for (int x = bounding_min.x; x <= bounding_max.x; x++) {
			Vec2i offset = { x - bounding_min.x, y - bounding_min.y };

			// 判断点(x,y)是否位于三角形内部或者三角形边缘
			// 当属于上边缘或者左边缘的时候，将e与1进行比较，从而跳过e=0的情况
			float e01 = edge_equation_01->Evaluate(offset.x, offset.y);
			if (e01 < (edge_equation_01->is_top_left ? 0 : 1)) continue;

			float e12 = edge_equation_12->Evaluate(offset.x, offset.y);
			if (e12 < (edge_equation_12->is_top_left ? 0 : 1)) continue;

			float e20 = edge_equation_20->Evaluate(offset.x, offset.y);
			if (e20 < (edge_equation_20->is_top_left ? 0 : 1)) continue;

			// 计算重心坐标barycentric coordinates
			float bc_p1 = e20 * edge_equation_20->w_reciprocal * bc_denominator;
			float bc_p2 = e01 * edge_equation_01->w_reciprocal * bc_denominator;
			float bc_p0 = 1 - bc_p1 - bc_p2;

			// 深度测试，使用反向z-buffer
			float depth = vertex[0]->position.z * bc_p0 + vertex[1]->position.z * bc_p1 + vertex[2]->position.z * bc_p2;
			if (1 - depth < depth_buffer_[y][x]) continue;
			depth_buffer_[y][x] = 1 - depth;


			// 准备为当前像素的各项 varying 进行插值
			ShaderContext attribute_current_vertex;

			ShaderContext& context_p0 = vertex[0]->context;
			ShaderContext& context_p1 = vertex[1]->context;
			ShaderContext& context_p2 = vertex[2]->context;

			// 插值各项 varying
			for (const auto& key : context_p0.varying_vec2f | std::views::keys) {
				float f0 = context_p0.varying_float[key];
				float f1 = context_p1.varying_float[key];
				float f2 = context_p2.varying_float[key];
				attribute_current_vertex.varying_float[key] = bc_p0 * f0 + bc_p1 * f1 + bc_p2 * f2;
			}

			for (const auto& key : context_p0.varying_vec2f | std::views::keys) {
				const Vec2f& f0 = context_p0.varying_vec2f[key];
				const Vec2f& f1 = context_p1.varying_vec2f[key];
				const Vec2f& f2 = context_p2.varying_vec2f[key];
				attribute_current_vertex.varying_vec2f[key] = bc_p0 * f0 + bc_p1 * f1 + bc_p2 * f2;
			}

			for (const auto& key : context_p0.varying_vec3f | std::views::keys) {
				const Vec3f& f0 = context_p0.varying_vec3f[key];
				const Vec3f& f1 = context_p1.varying_vec3f[key];
				const Vec3f& f2 = context_p2.varying_vec3f[key];
				attribute_current_vertex.varying_vec3f[key] = bc_p0 * f0 + bc_p1 * f1 + bc_p2 * f2;
			}

			for (const auto& key : context_p0.varying_vec4f | std::views::keys) {
				const Vec4f& f0 = context_p0.varying_vec4f[key];
				const Vec4f& f1 = context_p1.varying_vec4f[key];
				const Vec4f& f2 = context_p2.varying_vec4f[key];
				attribute_current_vertex.varying_vec4f[key] = bc_p0 * f0 + bc_p1 * f1 + bc_p2 * f2;
			}

			// 执行像素着色器
			Vec4f color = { 0.0f, 0.0f, 0.0f, 0.0f };
			if (pixel_shader_ != nullptr) {
				color = pixel_shader_(attribute_current_vertex);
			}
			SetPixel(x, y, color);
		}
	}

	// 绘制线框，再画一次避免覆盖
	if (render_frame_) {
		DrawLine(vertex[0]->screen_position_i.x, vertex[0]->screen_position_i.y, vertex[1]->screen_position_i.x, vertex[1]->screen_position_i.y);
		DrawLine(vertex[0]->screen_position_i.x, vertex[0]->screen_position_i.y, vertex[2]->screen_position_i.x, vertex[2]->screen_position_i.y);
		DrawLine(vertex[2]->screen_position_i.x, vertex[2]->screen_position_i.y, vertex[1]->screen_position_i.x, vertex[1]->screen_position_i.y);
	}

	return true;
}

