#pragma once

#include "Satellite.h"
#include "Config.h"
#include "Skybox.h"

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
	void SetupImGuiStyle();

	// Select a satellite *from	 m_SatelliteList*
	bool TrySelectSatellite(Satellite* satellite);


private:
	RenderTexture m_GlobeRenderTarget;
	bool m_Draw3DGlobe = true;
	void DrawGlobeView();		// Raylib logic -> 3D globe
	void DrawGlobeViewport();  // ImGui -> Display the render target
	void ControlGlobeCamera();
	Camera m_GlobeCamera;
	float m_GlobeZoom = 2.0f;
	bool m_FollowSatellite = false;
	bool m_CanControlGlobeCamera = false;


	RenderTexture m_MapRenderTarget;
	bool m_DrawWorldMap = true;
	void DrawMap();			// Raylib logic -> Lat/Long world map
	void DrawMapViewport(); // ImGui -> Display the render target 
	Vector2 m_MapWindowPosition;


	RenderTexture m_SkyRenderTarget;
	bool m_DrawSkyView = true;
	void DrawSkyView();
	void DrawSkyViewport();
	Camera m_SkyCamera;
	bool m_CanControlSkyCamera = false;


	bool m_DrawSatelliteList = true;
	void DrawSatelliteList();

	bool m_DrawPolarView = true;
	void DrawPolarView();

	bool m_DrawNextPasses = true;
	void DrawNextPasses();


	Config m_Config;

	Model m_EarthModel;
	Model m_SatelliteModel;
	Texture m_LocationBilboardMap;
	Texture m_SatelliteBilboard;
	Texture m_WorldMapTexture;
	Font m_DroidSansFont;
	Skybox m_Skybox;

	std::unordered_map<std::string, libsgp4::Tle> m_TLEs;
	libsgp4::DateTime m_StartTime;

	Satellite* m_SelectedSatellite = nullptr;
	std::vector<Satellite*> m_SatelliteList{};

	float m_MinElevationCoverage = 10.0f; // Minimum elevation in degrees used for the area of coverage 

private:
	struct PassDetails
	{
		libsgp4::DateTime AOS; // Acquisition Of Signal
		libsgp4::DateTime LOS; // Lost Of Signal
		float MaxElevation = 0.0f;

		std::vector<libsgp4::CoordTopocentric> PointsTopocentric;
	};
	
	// Pass prediction
	using PassData = std::unordered_map<std::string, std::vector<PassDetails>>;

	PassData m_PassData;
	std::vector<PassDetails> PredictPass(Satellite* sat, const libsgp4::DateTime& start, const libsgp4::DateTime& end, float minElevation = 10.0f);
	PassData PredictAllPasses(const libsgp4::DateTime& start, const libsgp4::DateTime& end, float minElevation = 10.0f);


};

}