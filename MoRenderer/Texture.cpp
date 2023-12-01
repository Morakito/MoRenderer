#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION	
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"



Texture::Texture(const char* filename)
{
	textureData = stbi_load(filename, &textureWidth, &textureHeight, &textureChannels, STBI_default);
}

Texture::~Texture()
{
	stbi_image_free(textureData);
}

ColorRGBA Texture::GetPixelColor(const int x, const int y) const
{
	ColorRGBA color(0.0f);
	if (x >= 0 && x < textureWidth &&
		y >= 0 && y < textureHeight) {
		const uint8_t* pixel_offset = textureData + (x + y * textureWidth) * textureChannels;
		color.r = pixel_offset[0] / 255.0f;
		color.g = pixel_offset[1] / 255.0f;
		color.b = pixel_offset[2] / 255.0f;
		color.a = textureChannels > 4 ? pixel_offset[3] / 255.0f : 1.0f;
	}
	return color;
}

ColorRGBA Texture::SampleBilinear(float x, float y) const
{
	int32_t x1 = floor(x);
	int32_t y1 = floor(y);

	int32_t x2 = ceil(x);
	int32_t y2 = ceil(y);

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



