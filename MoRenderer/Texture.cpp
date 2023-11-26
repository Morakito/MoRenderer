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

ColorRGBA Texture::GetPixelColor(int x, int y) const
{
	ColorRGBA color(0.0f);
	if (x >= 0 && x < textureWidth &&
		y >= 0 && y < textureHeight) {
		unsigned char* pixelOffset = textureData + (x + y * textureWidth) * textureChannels;
		color.r = pixelOffset[0] / 255.0f;
		color.g = pixelOffset[1] / 255.0f;
		color.b = pixelOffset[2] / 255.0f;
		color.a = textureChannels > 4 ? pixelOffset[3] / 255.0f : 1.0f;
	}
	return color;
}

ColorRGBA Texture::SampleBilinear(float x, float y)
{
	int32_t x1 = floor(x);
	int32_t y1 = floor(y);

	int32_t x2 = ceil(x);
	int32_t y2 = ceil(y);

	float tx = x - x1;
	float ty = y - y1;

	ColorRGBA c00 = GetPixelColor(x1, y1);
	ColorRGBA c01 = GetPixelColor(x2, y1);
	ColorRGBA c10 = GetPixelColor(x1, y2);
	ColorRGBA c11 = GetPixelColor(x2, y2);

	return BilinearInterpolation(c00, c01, c10, c11, tx, ty);
}

ColorRGBA Texture::BilinearInterpolation(ColorRGBA c00, ColorRGBA c01, ColorRGBA c10, ColorRGBA c11, float distx, float disty)
{
	ColorRGBA c0 = vector_lerp(c00, c01, distx);
	ColorRGBA c1 = vector_lerp(c10, c11, distx);
	ColorRGBA color = vector_lerp(c0, c1, disty);

	return color;
}



