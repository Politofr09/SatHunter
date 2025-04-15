#pragma once

#include "Satellite.h"
#include "Config.h"

#include <raylib.h>
#include <CoordTopocentric.h>
#include <CoordGeodetic.h>
#include <Observer.h>
#include <SGP4.h>

#include <unordered_map>
#include <array>

constexpr std::array<Color, 10> OrbitColors = {
	GREEN, BLUE, RED, PINK, WHITE, YELLOW, ORANGE, MAROON, MAGENTA, DARKGRAY
};

namespace SatHunter {

class Application
{
public:
	Application() = default;
	void Run();

private:

	void Initialize();
	void Tick();
	void Cleanup();

	void LoadResources();
	void ControlCamera();
	void SetupImGuiStyle();

	// Select a satellite *from	 m_SatelliteList*
	bool TrySelectSatellite(Satellite* satellite);


private:
	bool m_Draw3DGlobe = true;
	RenderTexture m_3DRenderTarget;
	void Draw3DView();		// Raylib logic -> 3D globe
	void Draw3DViewport();  // ImGui -> Display the render target

	RenderTexture m_MapRenderTarget;
	bool m_DrawWorldMap = true;
	void DrawMap();			// Raylib logic -> Lat/Long world map
	void DrawMapViewport(); // ImGui -> Display the render target 
	Vector2 m_MapWindowPosition;

	bool m_DrawSatelliteList = true;
	void DrawSatelliteList();

	bool m_DrawPolarView = true;
	void DrawPolarView();

	bool m_DrawSkyGlance = true;
	void DrawSkyGlance();

private:
	Config m_Config;

	Camera m_Camera;
	float m_Zoom = 2.0f;

	Model m_EarthModel;
	Model m_SatelliteModel;
	Texture m_LocationBilboardMap;
	Texture m_WorldMapTexture2k;
	Font m_DroidSansFont;

	std::unordered_map<std::string, libsgp4::Tle> m_TLEs;
	libsgp4::DateTime m_StartTime;

	Satellite* m_SelectedSatellite = nullptr;
	std::vector<Satellite*> m_SatelliteList;

	bool m_FollowSatellite = false;
	bool m_CanControlCamera = false;

private:
	// Pass prediction
	using PassData = std::unordered_map<std::string, std::vector<PassDetails>>;

	PassData m_PassData;
	std::vector<PassDetails> PredictPass(Satellite* sat, const libsgp4::DateTime& start, const libsgp4::DateTime& end, float minElevation = 10.0f);
	PassData PredictAllPasses(const libsgp4::DateTime& start, const libsgp4::DateTime& end, float minElevation = 10.0f);


};

}