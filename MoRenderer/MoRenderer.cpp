#include "MoRenderer.h"
#include "MoRenderer.h"

#include <optional>
#include <ranges>


// 顶点是否位于可视空间内部
// 此时vertex位于裁剪空间中，没有经过透视除法
// 使用DirectX中的设置，近裁剪平面会映射到z=0
bool IsVertexVisible(const Vec4f& vertex) {
	return
		fabs(vertex.x) <= vertex.w &&
		fabs(vertex.y) <= vertex.w &&
		vertex.z >= 0 && vertex.z <= vertex.w;
}

// 顶点是否位于某一侧裁剪平面内侧
// 此时vertex位于裁剪空间中，没有经过透视除法
// 使用DirectX中的设置，近裁剪平面会映射到z=0
bool IsInsidePlane(MoRenderer::ClipPlane clip_plane, const Vec4f& vertex)
{
	bool result = false;
	switch (clip_plane)
	{
		// 防止进行透视除法时出现数值溢出
	case MoRenderer::W_Plane:
		result = vertex.w >= kEpsilon;
		break;
	case MoRenderer::X_RIGHT:
		result = vertex.x <= vertex.w;
		break;
	case MoRenderer::X_LEFT:
		result = vertex.x >= -vertex.w;
		break;
	case MoRenderer::Y_TOP:
		result = vertex.y <= vertex.w;
		break;
	case MoRenderer::Y_BOTTOM:
		result = vertex.y >= -vertex.w;
		break;
	case MoRenderer::Z_Near:
		result = vertex.z >= 0;
		break;
	case MoRenderer::Z_FAR:
		result = vertex.z <= vertex.w;
		break;

	default:;
	}

	return  result;
}

// 获取裁剪空间下，与裁剪平面的比例系数，用于生成与裁剪平面的交点
float GetIntersectRatio(MoRenderer::ClipPlane clip_plane, const Vec4f& pre_vertex, const Vec4f& cur_vertex)
{
	float intersect_ratio = 1.0f;
	switch (clip_plane)
	{
	case MoRenderer::X_RIGHT:
		intersect_ratio = (pre_vertex.w - pre_vertex.x) /
			((pre_vertex.w - pre_vertex.x) - (cur_vertex.w - cur_vertex.x));
		break;
	case MoRenderer::X_LEFT:
		intersect_ratio = (pre_vertex.w + pre_vertex.x) /
			((pre_vertex.w + pre_vertex.x) - (cur_vertex.w + cur_vertex.x));
		break;
	case MoRenderer::Y_TOP:
		intersect_ratio = (pre_vertex.w - pre_vertex.y) /
			((pre_vertex.w - pre_vertex.y) - (cur_vertex.w - cur_vertex.y));
		break;
	case MoRenderer::Y_BOTTOM:
		intersect_ratio = (pre_vertex.w + pre_vertex.y) /
			((pre_vertex.w + pre_vertex.y) - (cur_vertex.w + cur_vertex.y));
		break;
	case MoRenderer::Z_Near:
		// 由于使用DirectX中的投影矩阵，将near plane映射到z=0上，因此比例系数的计算公式有所改变
		intersect_ratio = (pre_vertex.z / pre_vertex.w) /
			(pre_vertex.z / pre_vertex.w - cur_vertex.z / cur_vertex.w);
		break;
	case MoRenderer::Z_FAR:
		intersect_ratio = (pre_vertex.w + pre_vertex.z) /
			((pre_vertex.w + pre_vertex.z) - (cur_vertex.w + cur_vertex.z));
		break;
	default:;
	}

	return  intersect_ratio;
}

// 详见https://fabiensanglard.net/polygon_codec/
int MoRenderer::ClipWithPlane(ClipPlane clip_plane, Vertex vertex[3])
{
	int out_vertex_count = 0;
	constexpr int vertex_count = 3;

	for (int i = 0; i < vertex_count; i++)
	{
		const int cur_index = i;
		const int pre_index = (i - 1 + vertex_count) % vertex_count;

		Vec4f cur_vertex = vertex[cur_index].position;
		Vec4f pre_vertex = vertex[pre_index].position;

		const bool is_cur_inside = IsInsidePlane(clip_plane, cur_vertex);
		const bool is_pre_inside = IsInsidePlane(clip_plane, pre_vertex);

		if (is_cur_inside ^ is_pre_inside)
		{
			const float ratio = GetIntersectRatio(clip_plane, pre_vertex, cur_vertex);
			Vertex& new_vertex = VertexLerp(vertex[pre_index], vertex[cur_index], ratio);
			clip_vertex_[out_vertex_count] = &new_vertex;
			out_vertex_count++;
		}

		if (is_cur_inside)
		{
			clip_vertex_[out_vertex_count] = &vertex[cur_index];
			out_vertex_count++;
		}
	}

	return  out_vertex_count;
}


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

	ClearFrameBuffer(true, true);
}

void MoRenderer::ClearFrameBuffer(bool clear_color_buffer, bool clear_depth_buffer) const
{
	if (clear_color_buffer && color_buffer_)
	{
		const ColorRGBA32Bit color_32_bit = vector_to_32bit_color(color_background_);

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

	if (clear_depth_buffer && depth_buffer_) {
		for (int j = 0; j < frame_buffer_height_; j++) {
			for (int i = 0; i < frame_buffer_width_; i++)
				depth_buffer_[j][i] = 0.0f;
		}
	}
}

void MoRenderer::SetBuffer(uint8_t* buffer, const int x, const int y, const Vec4f& color) const
{
	if (x < 0 || x>frame_buffer_width_ - 1) return;
	if (y < 0 || y>frame_buffer_height_ - 1) return;

	const ColorRGBA32Bit color_32_bit = vector_to_32bit_color(color);
	const int base_address = frame_buffer_width_ * (4 * y) + 4 * x;
	//32 bit位图存储顺序，从低到高依次为BGRA
	buffer[base_address] = color_32_bit.b;
	buffer[base_address + 1] = color_32_bit.g;
	buffer[base_address + 2] = color_32_bit.r;
	buffer[base_address + 3] = color_32_bit.a;
}

MoRenderer::Vertex& MoRenderer::VertexLerp(Vertex& vertex_p0, Vertex& vertex_p1, const float ratio)
{
	auto* vertex = new Vertex();
	vertex->position = vector_lerp(vertex_p0.position, vertex_p1.position, ratio);

	Varings& context = vertex->context;
	Varings& context_p0 = vertex_p0.context;
	Varings& context_p1 = vertex_p1.context;

	for (const auto& key : context_p0.varying_float | std::views::keys) {
		float f0 = context_p0.varying_float[key];
		float f1 = context_p1.varying_float[key];
		context.varying_float[key] = std::lerp(f0, f1, ratio);
	}
	for (const auto& key : context_p0.varying_vec2f | std::views::keys) {
		const Vec2f& f0 = context_p0.varying_vec2f[key];
		const Vec2f& f1 = context_p1.varying_vec2f[key];
		context.varying_vec2f[key] = vector_lerp(f0, f1, ratio);
	}

	for (const auto& key : context_p0.varying_vec3f | std::views::keys) {
		const Vec3f& f0 = context_p0.varying_vec3f[key];
		const Vec3f& f1 = context_p1.varying_vec3f[key];
		context.varying_vec3f[key] = vector_lerp(f0, f1, ratio);

	}

	for (const auto& key : context_p0.varying_vec4f | std::views::keys) {
		const Vec4f& f0 = context_p0.varying_vec4f[key];
		const Vec4f& f1 = context_p1.varying_vec4f[key];
		context.varying_vec4f[key] = vector_lerp(f0, f1, ratio);
	}



	return  *vertex;
}

void MoRenderer::DrawWireFrame(Vertex* vertex[3]) const
{
	DrawLine(vertex[0]->screen_position_i.x, vertex[0]->screen_position_i.y, vertex[1]->screen_position_i.x, vertex[1]->screen_position_i.y);
	DrawLine(vertex[1]->screen_position_i.x, vertex[1]->screen_position_i.y, vertex[2]->screen_position_i.x, vertex[2]->screen_position_i.y);
	DrawLine(vertex[2]->screen_position_i.x, vertex[2]->screen_position_i.y, vertex[0]->screen_position_i.x, vertex[0]->screen_position_i.y);
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

void MoRenderer::DrawSkybox()
{
	if (color_buffer_ == nullptr || vertex_shader_ == nullptr) return;

	// 顶点变换
	for (int k = 0; k < 3; k++) {
		vertex_[k].context.varying_float.clear();
		vertex_[k].context.varying_vec2f.clear();
		vertex_[k].context.varying_vec3f.clear();
		vertex_[k].context.varying_vec4f.clear();

		// 执行顶点着色程序，返回裁剪空间中的顶点坐标，此时没有进行透视除法
		vertex_[k].position = vertex_shader_(k, vertex_[k].context);
	}

	Vertex* raster_vertex[3] = { &vertex_[0], &vertex_[1], &vertex_[2] };
	// 执行后续顶点处理
	for (int k = 0; k < 3; k++) {
		Vertex* current_vertex = raster_vertex[k];

		// 透视除法
		current_vertex->w_reciprocal = 1.0f / current_vertex->position.w;
		current_vertex->position *= current_vertex->w_reciprocal;

		// 屏幕映射：计算屏幕坐标（窗口坐标。详见RTR4 章节2.3.4
		current_vertex->screen_position_f.x = (current_vertex->position.x + 1.0f) * static_cast<float>(frame_buffer_width_ - 1) * 0.5f;
		current_vertex->screen_position_f.y = (current_vertex->position.y + 1.0f) * static_cast<float>(frame_buffer_height_ - 1) * 0.5f;

		// 计算整数屏幕坐标：d = floor(c)
		current_vertex->screen_position_i.x = static_cast<int>(floor(current_vertex->screen_position_f.x));
		current_vertex->screen_position_i.y = static_cast<int>(floor(current_vertex->screen_position_f.y));

		//计算整数屏幕坐标：c = d + 0.5
		current_vertex->screen_position_f.x = current_vertex->screen_position_i.x + 0.5f;
		current_vertex->screen_position_f.y = current_vertex->screen_position_i.y + 0.5f;
	}

	RasterizeTriangle(raster_vertex);
}

void MoRenderer::DrawMesh() {
	if (color_buffer_ == nullptr || vertex_shader_ == nullptr) return;

	// 顶点变换
	for (int k = 0; k < 3; k++) {
		vertex_[k].context.varying_float.clear();
		vertex_[k].context.varying_vec2f.clear();
		vertex_[k].context.varying_vec3f.clear();
		vertex_[k].context.varying_vec4f.clear();

		// 执行顶点着色程序，返回裁剪空间中的顶点坐标，此时没有进行透视除法
		vertex_[k].position = vertex_shader_(k, vertex_[k].context);
		vertex_[k].has_transformed = false;
	}

	/*
	* 裁剪空间中的背面剔除：
	*
	* 在观察空间中进行判断，观察空间使用右手坐标系，即相机看向z轴负方向
	* 判断三角形朝向，剔除背对相机的三角形
	* 由于相机看向z轴负方向，因此三角形法线的z分量为负，说明背对相机
	*
	* 顶点顺序：
	* obj格式中默认的顶点顺序是逆时针，即顶点v1，v2，v3按照逆时针顺序排列
	*/
	const Vec4f vector_01 = vertex_[1].position - vertex_[0].position;
	const Vec4f vector_02 = vertex_[2].position - vertex_[0].position;
	const Vec4f normal = vector_cross(vector_01, vector_02);
	if (normal.z <= 0) return;

	int out_vertex_count = 0;
	if (IsInsidePlane(Z_Near, vertex_[0].position) &&
		IsInsidePlane(Z_Near, vertex_[1].position) &&
		IsInsidePlane(Z_Near, vertex_[2].position))
	{
		out_vertex_count = 3;
		clip_vertex_[0] = &vertex_[0];
		clip_vertex_[1] = &vertex_[1];
		clip_vertex_[2] = &vertex_[2];
	}
	else
	{	// 在裁剪空间中，针对近裁剪平面进行clip
		out_vertex_count = ClipWithPlane(Z_Near, vertex_);
	}



	for (int i = 0; i < out_vertex_count - 2; i++)
	{
		Vertex* raster_vertex[3] = { clip_vertex_[0], clip_vertex_[i + 1], clip_vertex_[i + 2] };

		// 执行后续顶点处理
		for (int k = 0; k < 3; k++) {
			Vertex* current_vertex = raster_vertex[k];

			if (current_vertex->has_transformed)continue;
			current_vertex->has_transformed = true;

			// 透视除法
			current_vertex->w_reciprocal = 1.0f / current_vertex->position.w;
			current_vertex->position *= current_vertex->w_reciprocal;

			// 屏幕映射：计算屏幕坐标（窗口坐标。详见RTR4 章节2.3.4
			current_vertex->screen_position_f.x = (current_vertex->position.x + 1.0f) * static_cast<float>(frame_buffer_width_ - 1) * 0.5f;
			current_vertex->screen_position_f.y = (current_vertex->position.y + 1.0f) * static_cast<float>(frame_buffer_height_ - 1) * 0.5f;

			// 计算整数屏幕坐标：d = floor(c)
			current_vertex->screen_position_i.x = static_cast<int>(floor(current_vertex->screen_position_f.x));
			current_vertex->screen_position_i.y = static_cast<int>(floor(current_vertex->screen_position_f.y));

			//计算整数屏幕坐标：c = d + 0.5
			current_vertex->screen_position_f.x = current_vertex->screen_position_i.x + 0.5f;
			current_vertex->screen_position_f.y = current_vertex->screen_position_i.y + 0.5f;

		}

		RasterizeTriangle(raster_vertex);
	}

}

void MoRenderer::RasterizeTriangle(Vertex* vertex[3])
{
	// 三角形屏幕空间中的外接矩形
	Vec2i bounding_min(100000, 100000), bounding_max(-100000, -100000);

	// 寻找外界矩形范围
	for (size_t i = 0; i < 3; i++)
	{
		Vec2i screen_position_i = vertex[i]->screen_position_i;
		// 更新外接矩形
		bounding_min.x = Min(bounding_min.x, screen_position_i.x);
		bounding_max.x = Max(bounding_max.x, screen_position_i.x);
		bounding_min.y = Min(bounding_min.y, screen_position_i.y);
		bounding_max.y = Max(bounding_max.y, screen_position_i.y);
	}

	bounding_min.x = Between(0, frame_buffer_width_ - 1, bounding_min.x);
	bounding_max.x = Between(0, frame_buffer_width_ - 1, bounding_max.x);
	bounding_min.y = Between(0, frame_buffer_height_ - 1, bounding_min.y);
	bounding_max.y = Between(0, frame_buffer_height_ - 1, bounding_max.y);


	// 只绘制线框，不绘制像素，直接退出
	if (render_frame_ && !render_pixel_) {
		DrawWireFrame(vertex);
		return;
	}

	// 保存三个端点位置
	Vec2i p0 = vertex[0]->screen_position_i;
	Vec2i p1 = vertex[1]->screen_position_i;
	Vec2i p2 = vertex[2]->screen_position_i;

	// 构建边缘方程
	Vec2i bottom_left_point = bounding_min;
	edge_equation_[0].Initialize(p1, p2, bottom_left_point, vertex[0]->w_reciprocal);
	edge_equation_[1].Initialize(p2, p0, bottom_left_point, vertex[1]->w_reciprocal);
	edge_equation_[2].Initialize(p0, p1, bottom_left_point, vertex[2]->w_reciprocal);

	// 迭代三角形外接矩形中的所有点
	for (int y = bounding_min.y; y <= bounding_max.y; y++) {
		for (int x = bounding_min.x; x <= bounding_max.x; x++) {
			Vec2i offset = { x - bounding_min.x, y - bounding_min.y };

			// 判断点(x,y)是否位于三角形内部或者三角形边缘
			// 左上边：e >= 0，若e < 0即跳过
			// 右下边：e > 0，将e <= 0转换为e < 1
			float e0 = edge_equation_[0].Evaluate(offset.x, offset.y);
			if (e0 < (edge_equation_[0].is_top_left ? 0 : 1)) continue;
			
			float e1 = edge_equation_[1].Evaluate(offset.x, offset.y);
			if (e1 < (edge_equation_[1].is_top_left ? 0 : 1)) continue;

			float e2 = edge_equation_[2].Evaluate(offset.x, offset.y);
			if (e2 < (edge_equation_[2].is_top_left ? 0 : 1)) continue;

			// 计算重心坐标
			float bc_denominator = e0 + e1 + e2;
			bc_denominator = 1.0f / bc_denominator;

			float bc_p0 = e0 * bc_denominator;
			float bc_p1 = e1 * bc_denominator;
			float bc_p2 = e2 * bc_denominator;

			// 计算透视正确的重心坐标
			float bc_correct_denominator =
				e0 * edge_equation_[0].w_reciprocal +
				e1 * edge_equation_[1].w_reciprocal +
				e2 * edge_equation_[2].w_reciprocal;
			bc_correct_denominator = 1.0f / bc_correct_denominator;

			float bc_correct_p0 = e0 * edge_equation_[0].w_reciprocal * bc_correct_denominator;
			float bc_correct_p1 = e1 * edge_equation_[1].w_reciprocal * bc_correct_denominator;
			float bc_correct_p2 = e2 * edge_equation_[2].w_reciprocal * bc_correct_denominator;


			// 对深度进行插值，进行深度测试，使用反向z-buffer
			float depth =
				vertex[0]->position.z * bc_p0 +
				vertex[1]->position.z * bc_p1 +
				vertex[2]->position.z * bc_p2;

			if (1.0f - depth <= depth_buffer_[y][x]) continue;
			depth_buffer_[y][x] = 1.0f - depth;

			// 准备为当前像素的各项 varying 进行插值

			Varings& context_p0 = vertex[0]->context;
			Varings& context_p1 = vertex[1]->context;
			Varings& context_p2 = vertex[2]->context;

			// 插值各项 varying
			if (!context_p0.varying_float.empty()) {
				for (const auto& key : context_p0.varying_float | std::views::keys) {
					float f0 = context_p0.varying_float[key];
					float f1 = context_p1.varying_float[key];
					float f2 = context_p2.varying_float[key];
					current_varings_.varying_float[key] = bc_correct_p0 * f0 + bc_correct_p1 * f1 + bc_correct_p2 * f2;
				}
			}

			if (!context_p0.varying_vec2f.empty()) {
				for (const auto& key : context_p0.varying_vec2f | std::views::keys) {
					const Vec2f& f0 = context_p0.varying_vec2f[key];
					const Vec2f& f1 = context_p1.varying_vec2f[key];
					const Vec2f& f2 = context_p2.varying_vec2f[key];
					current_varings_.varying_vec2f[key] = bc_correct_p0 * f0 + bc_correct_p1 * f1 + bc_correct_p2 * f2;
				}
			}


			if (!context_p0.varying_vec3f.empty()) {
				for (const auto& key : context_p0.varying_vec3f | std::views::keys) {
					const Vec3f& f0 = context_p0.varying_vec3f[key];
					const Vec3f& f1 = context_p1.varying_vec3f[key];
					const Vec3f& f2 = context_p2.varying_vec3f[key];
					current_varings_.varying_vec3f[key] = bc_correct_p0 * f0 + bc_correct_p1 * f1 + bc_correct_p2 * f2;
				}
			}


			if (!context_p0.varying_vec4f.empty()) {
				for (const auto& key : context_p0.varying_vec4f | std::views::keys) {
					const Vec4f& f0 = context_p0.varying_vec4f[key];
					const Vec4f& f1 = context_p1.varying_vec4f[key];
					const Vec4f& f2 = context_p2.varying_vec4f[key];
					current_varings_.varying_vec4f[key] = bc_correct_p0 * f0 + bc_correct_p1 * f1 + bc_correct_p2 * f2;
				}
			}

			// 执行像素着色器
			Vec4f color = { 1.0f };
			if (pixel_shader_ != nullptr) {
				color = pixel_shader_(current_varings_);
			}
			SetPixel(x, y, color);
		}
	}

	// 绘制线框，再画一次避免覆盖
	if (render_frame_) {
		DrawWireFrame(vertex);
	}
}
