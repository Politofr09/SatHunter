#pragma once

#include <string>
#include <raylib.h>

namespace SatHunter {

class Skybox
{
public:
	Skybox() = default;

	void Load(std::string hdrFilename);
	void Draw();

private:
	TextureCubemap m_TextureCubemap;
	Shader m_CubemapShader;
	Model m_Skybox;
};

}