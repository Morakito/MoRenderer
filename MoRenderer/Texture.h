#ifndef TEXTURE_H
#define TEXTURE_H

#include "math.h"

class Texture
{
public:
	Texture(const std::string& file_name);
	~Texture();

	// 纹理采样
	Vec4f Sample2D(float u, float v) const;

	Vec4f Sample2D( Vec2f uv) const;


private:

	ColorRGBA GetPixelColor(int x, int y) const;
	ColorRGBA SampleBilinear(float x, float y) const;
	static ColorRGBA BilinearInterpolation(const ColorRGBA& color00, const ColorRGBA& color01, const ColorRGBA& color10, const ColorRGBA& color11, float t_x, float t_y);

public:
	int texture_width_;					// 纹理宽度
	int texture_height_;				// 纹理高度
	int texture_channels_;				// 纹理通道数

	bool has_data_;						// 是否存在数据，即是否成功加载贴图
	unsigned char* texture_data_;		// 实际的图像数据
};


#endif // !TEXTURE_H
