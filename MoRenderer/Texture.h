#ifndef TEXTURE_H
#define TEXTURE_H

#include "math.h"

// ����������ͼ
class Texture
{
public:
	Texture(const std::string& file_name);
	~Texture();

	// �������
	Vec4f Sample2D(float u, float v) const;
	Vec4f Sample2D(Vec2f uv) const;

private:

	ColorRGBA GetPixelColor(int x, int y) const;
	ColorRGBA SampleBilinear(float x, float y) const;
	static ColorRGBA BilinearInterpolation(const ColorRGBA& color00, const ColorRGBA& color01, const ColorRGBA& color10, const ColorRGBA& color11, float t_x, float t_y);

public:
	int texture_width_;					// ������
	int texture_height_;				// ����߶�
	int texture_channels_;				// ����ͨ����

	bool has_data_;						// �Ƿ�������ݣ����Ƿ�ɹ�������ͼ
	unsigned char* texture_data_;		// ʵ�ʵ�ͼ������
};

// ��������ͼ��������պ�
class CubeMap
{
public:
	CubeMap(const std::string& file_folder);
	~CubeMap();

	struct CubeMapUV
	{
		int face_id;
		Vec2f uv;
	};

	Vec3f Sample(Vec3f& direction) const;

	static CubeMapUV& CalculateCubeMapUV(Vec3f& direction);

public:
	Texture* cubemap_[6];
};


#endif // !TEXTURE_H
