#ifndef TEXTURE_H
#define TEXTURE_H

#include "math.h"

enum TextureType
{
	kTextureTypeBaseColor,
	kTextureTypeNormal,
	kTextureTypeRoughness,
	kTextureTypeMetallic,
	kTextureTypeOcclusion,
	kTextureTypeEmission
};

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

// ��������ͼ
class CubeMap
{

public:
	struct CubeMapUV
	{
		int face_id;
		Vec2f uv;
	};

	enum CubeMapType
	{
		kSkybox,
		kIrradianceMap,
		kSpecularMap
	};

public:
	CubeMap(const std::string& file_folder, CubeMapType cube_map_type, int mipmap_level = 0);
	~CubeMap();
	Vec3f Sample(Vec3f& direction) const;

	static CubeMapUV& CalculateCubeMapUV(Vec3f& direction);

public:
	Texture* cubemap_[6];
	CubeMapType cube_map_type_;
};

// Ԥ���˵Ļ�����ͼ
class SpecularCubeMap
{
public:
	SpecularCubeMap(const std::string& file_folder, CubeMap::CubeMapType cube_map_type);


public:
	static constexpr int max_mipmap_level_ = 10;
	CubeMap* prefilter_maps_[max_mipmap_level_];
};


class IBLMap
{

public:
	IBLMap() = default;
	IBLMap(const std::string& skybox_path);

public:
	CubeMap* skybox_cubemap_;
	CubeMap* irradiance_cubemap_;
	SpecularCubeMap* specular_cubemap_;
	Texture* brdf_lut_;

	std::string skybox_name_;
	std::string skybox_folder_;
};


#endif // !TEXTURE_H
