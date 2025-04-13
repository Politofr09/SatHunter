#pragma once

#include "Satellite.h"
#include "Config.h"

#include <raylib.h>
#include <CoordTopocentric.h>
#include <CoordGeodetic.h>
#include <Observer.h>
#include <SGP4.h>

#include <unordered_map>

const libsgp4::Tle DefaultTLE(
	"UK-DMC 2",
	"1 35683U 09041C   12289.23158813  .00000484  00000-0  89219-4 0  5863",
	"2 35683  98.0221 185.3682 0001499 100.5295 259.6088 14.69819587172294");

namespace SatHunter {

class Application
{
public:
	Application()
		: m_SelectedTLE(DefaultTLE),
		sgp4(DefaultTLE)
	{
	}

	void Run();

private:

	void Initialize();
	void Tick();
	void Cleanup();

	void LoadResources();
	void ControlCamera();
	void SetupImGuiStyle();

	bool TrySetSatellite(const std::string& name);


private:
	bool m_Draw3DGlobe = true;
	RenderTexture m_3DRenderTarget;
	void Draw3DView();		// Raylib logic -> 3D globe
	void Draw3DViewport();  // ImGui -> Display the render target

	RenderTexture m_MapRenderTarget;
	bool m_DrawWorldMap = true;
	void DrawMap();			// Raylib logic -> Lat/Long world map
	void DrawMapViewport(); // ImGui -> Display the render target 

	bool m_DrawSatelliteList = true;
	void DrawSatelliteList();

private:
	Config m_Config;

	Camera m_Camera;
	float m_Zoom = 2.0f;

	Model m_EarthModel;
	Model m_SatelliteModel;
	Texture m_LocationBilboardMap;
	Texture m_WorldMapTexture2k;

	libsgp4::SGP4 sgp4;
	std::unordered_map<std::string, libsgp4::Tle> m_TLEs;
	std::string m_SelectedSatName;
	libsgp4::Tle m_SelectedTLE;

	OrbitPath m_OrbitPath;

	bool m_FollowSatellite = false;
	bool m_CanControlCamera = false;
};

}