#ifndef SCENE_H
#define SCENE_H

#include "Texture.h"
#include "Model.h"
#include "Shader.h"
#include "Window.h"


inline std::vector<std::string> model_paths =
{
	"../assets/helmet/helmet.obj",
	"../assets/Cerberus/Cerberus.obj"
};

inline std::vector<Mat4x4f> model_matrices =
{
	matrix_set_rotate(1.0f, 0.0f, 0.0f, -kPi * 0.5f) * matrix_set_scale(1, 1, 1),
	matrix_set_scale(2, 2, 2)
};

inline std::vector<std::string> skybox_paths =
{
	"../assets/kloofendal_48d_partly_cloudy_puresky/kloofendal_48d_partly_cloudy_puresky.hdr",
	"../assets/spruit_sunrise/spruit_sunrise.hdr",
	"../assets/brown_photostudio/brown_photostudio.hdr",
	"../assets/autumn_forest_04/autumn_forest_04.hdr"
};


class Scene
{
public:
	Scene();
	~Scene();

	// ¸ºÔðshaderµÄÇÐ»»
	void HandleKeyEvents(PBRShader* pbr_shader, BlinnPhongShader* blinn_phong_shader);

	void UpdateShaderInfo(IShader* shader) const;

	void LoadNextModel();
	void LoadPrevModel();

	void LoadNextIBLMap();
	void LoadPrevIBLMap();
public:
	std::vector< Model* >models_;
	Model* current_model_;
	int total_model_count_;
	int current_model_index_;

	std::vector< IBLMap* >iblmaps_;
	IBLMap* current_iblmap_;
	int total_iblmap_count_;
	int current_iblmap_index_;

	Window* window_;
	ShaderType current_shader_type_;

};






#endif // !SCENE_H

