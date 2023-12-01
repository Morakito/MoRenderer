#ifndef TEXTURE_H
#define TEXTURE_H

#include "math.h"

class Texture
{
public:
	Texture(const char* filename);
	~Texture();

	// ÎÆÀí²ÉÑù
	Vec4f Sample2D(float u, float v) {
		return SampleBilinear(u * textureWidth + 0.5f, v * textureHeight + 0.5f);
	}

	Vec4f Sample2D(const Vec2f& uv) {
		return Sample2D(uv.x, uv.y);
	}



private:

	ColorRGBA GetPixelColor(int x, int y) const;
	ColorRGBA SampleBilinear(float x, float y) const;
	static ColorRGBA BilinearInterpolation(const ColorRGBA& color00, const ColorRGBA& color01, const ColorRGBA& color10, const ColorRGBA& color11, float t_x, float t_y);

public:
	int textureWidth, textureHeight, textureChannels;
	unsigned char* textureData;
};


#endif // !TEXTURE_H
