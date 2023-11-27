#ifndef CAMERA_H
#define CAMERA_H

#include "math.h"

#include "Shader.h"

class Camera
{
public:
	Camera(Vec3f _position, Vec3f _target, Vec3f _up, float _fov, float _aspect);
	~Camera();

	// 处理输入事件
	void HandleInputEvents();

	// 更新uniformbuffer中的矩阵
	void UpdateUniformBuffer(UniformBuffer* uniformbuffer);

private:
	// 更新相机姿态
	void UpdataCameraPose();
	void HandleMouseEvents();
	void HandleKeyEvents();


public:
	Vec3f position;					// 相机的世界空间位置
	Vec3f target;					// 相机看向的世界空间位置
	Vec3f up;						// 相机的up向量

	/*
		相机坐标系的轴
		axisX：正方向指向屏幕右侧
		axisY：正方向指向屏幕上侧
		axisZ：正方向指向屏幕
	*/
	Vec3f axisX, axisY, axisZ;		

	float fov;						// 相机FOV
	float aspect;					// 长宽比

	float nearPlane;				// 近裁剪平面
	float farPlane;					// 远裁剪平面
};

#endif // !CAMERA_H
