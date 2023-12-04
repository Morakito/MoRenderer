#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION	
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"



Texture::Texture(const std::string& file_name)
{
	texture_data_ = stbi_load(file_name.c_str(), &texture_width_, &texture_height_, &texture_channels_, STBI_default);
	has_data_ = (texture_data_ != nullptr);

	std::cout << has_data_ << std::endl;
}

Texture::~Texture()
{
	stbi_image_free(texture_data_);
}

Vec4f Texture::Sample2D(float u, float v) const
{
	if (!has_data_) return { 1.0f };

	u = fmod(u, 1);
	v = fmod(v, 1);

	return SampleBilinear(u * texture_width_, v * texture_height_);
}

Vec4f Texture::Sample2D(Vec2f uv) const
{
	uv.x = fmod(uv.x, 1);
	uv.y = fmod(uv.y, 1);

	return Sample2D(uv.x, uv.y);
}

ColorRGBA Texture::GetPixelColor(const int x, const int y) const
{
	ColorRGBA color(0.0f);
	if (x >= 0 && x < texture_width_ &&
		y >= 0 && y < texture_height_) {
		const uint8_t* pixel_offset = texture_data_ + (x + y * texture_width_) * texture_channels_;
		color.r = pixel_offset[0] / 255.0f;
		color.g = pixel_offset[1] / 255.0f;
		color.b = pixel_offset[2] / 255.0f;
		color.a = texture_channels_ > 4 ? pixel_offset[3] / 255.0f : 1.0f;
	}
	return color;
}

ColorRGBA Texture::SampleBilinear(const float x, const float y) const
{
	const auto x1 = static_cast<int>(floor(x));
	const auto y1 = static_cast<int>(floor(y));

	const auto x2 = static_cast<int> (ceil(x));
	const auto y2 = static_cast<int> (ceil(y));

	const float t_x = x - x1;
	const float t_y = y - y1;

	const ColorRGBA color00 = GetPixelColor(x1, y1);
	const ColorRGBA color01 = GetPixelColor(x2, y1);
	const ColorRGBA color10 = GetPixelColor(x1, y2);
	const ColorRGBA color11 = GetPixelColor(x2, y2);

	return BilinearInterpolation(color00, color01, color10, color11, t_x, t_y);
}

ColorRGBA Texture::BilinearInterpolation(const ColorRGBA& color00, const ColorRGBA& color01, const ColorRGBA& color10, const ColorRGBA& color11, float t_x, float t_y)
{
	const ColorRGBA color0 = vector_lerp(color00, color01, t_x);
	const ColorRGBA color1 = vector_lerp(color10, color11, t_x);
	ColorRGBA color = vector_lerp(color0, color1, t_y);

	return color;
}



