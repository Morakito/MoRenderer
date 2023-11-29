#ifndef CAMERA_H
#define CAMERA_H

#include "math.h"

#include "Shader.h"

class Camera
{
public:
	Camera(Vec3f position, Vec3f target, Vec3f up, float fov, float aspect);
	~Camera();

	// 处理输入事件
	void HandleInputEvents();

	// 更新uniform buffer中的矩阵
	void UpdateUniformBuffer(UniformBuffer* uniform_buffer);

private:
	// 更新相机姿态
	void UpdateCameraPose();
	void HandleMouseEvents();
	void HandleKeyEvents();


public:
	Vec3f position_;					// 相机的世界空间位置
	Vec3f target_;					// 相机看向的世界空间位置
	Vec3f up_;						// 相机的up向量

	/*
		相机坐标系的轴
		axis_r：正方向指向屏幕右侧
		axis_u：正方向指向屏幕上侧
		axis_v：正方向指向屏幕
	*/
	Vec3f axis_r, axis_u, axis_v;

	float fov_;						// 相机FOV
	float aspect_;					// 长宽比

	float near_plane_;				// 近裁剪平面
	float far_plane_;					// 远裁剪平面
};

#endif // !CAMERA_H
