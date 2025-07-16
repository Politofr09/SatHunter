#include "Application.h"
#include "TLEUtils.h"

#include <rlImGui.h>
#include <extras/IconsFontAwesome6.h>
#include <extras/FA6FreeSolidFontData.h>
#include <raymath.h>
#include <imgui.h>

#include <chrono>

namespace SatHunter {

void Application::Run()
{
	Initialize();
		
	while (!WindowShouldClose())
	{
		Tick();
	}

	Cleanup();
}

void Application::Initialize()
{
	m_Config = LoadConfigFile();

	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
	InitWindow(1280, 720, "SatHunter: No satellite selected yet");
	MaximizeWindow();
	SetTargetFPS(60);

	m_Camera = {
		.position = { -10, 10, -10 },
		.target = { 0, 0, 0 },
		.up = { 0, 1, 0 },
		.fovy = 45.0f,
		.projection = CAMERA_PERSPECTIVE,
	};

	rlImGuiBeginInitImGui();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
	io.FontDefault = io.Fonts->AddFontFromFileTTF("res/droid-sans/DroidSans.ttf", 16.0f);

	rlImGuiEndInitImGui();

	SetupImGuiStyle();

	// TLE fetching and caching
	CheckTLEs(m_Config.General.TleUrl);
	m_TLEs = LoadTLEs();

	m_StartTime = libsgp4::DateTime::Now();

	LoadResources();

	for (const std::string& sat : m_Config.Tracker.Satellites)
	{
		// Create a satellite for every TLE that has 'sat' name
		m_SatelliteList.push_back(new Satellite(m_TLEs[sat]));
	}
	
	TrySelectSatellite(m_SatelliteList[0]);

	m_PassData = PredictAllPasses(m_StartTime, m_StartTime.AddHours(24));
}

void Application::Tick()
{
	if (m_Draw3DGlobe) Draw3DView();
	if (m_DrawWorldMap) DrawMap();

	// Update the orbits and the passes every 2 minutes
	libsgp4::DateTime now = libsgp4::DateTime::Now();
	libsgp4::TimeSpan elapsed = now - m_StartTime;

	if (elapsed.TotalMinutes() >= 2.0)
	{
		m_StartTime = now;

		m_PassData = PredictAllPasses(m_StartTime, m_StartTime.AddHours(24));
		
		for (auto& sat : m_SatelliteList)
		{
			sat->RecalculateOrbit();
		}
	}

	BeginDrawing();

		ClearBackground(DARKGRAY);

		// Draw maybe a logo or something
		// Like vscode ...

		rlImGuiBegin();


		ImVec2 ViewportDimensions;

		ViewportDimensions.x = GetScreenWidth();
		ViewportDimensions.y = GetScreenHeight();

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ViewportDimensions);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoBringToFrontOnFocus |                 // we just want to use this window as a host for the menubar and docking
			ImGuiWindowFlags_NoNavFocus |                                                      // so turn off everything that would make it act like a window
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoBackground;                                                      // we want our game content to show through this window, so turn off the background.

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));                          // we don't want any padding for windows docked to this window frame

		bool show = (ImGui::Begin("Main", NULL, windowFlags));                                   // show the "window"
		ImGui::PopStyleVar();                                                                    // restore the style so inner windows have fames

		// create a docking space inside our inner window that lets prevents anything from docking in the central node (so we can see our game content)
		ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
		if (show)
		{
			// Do a menu bar with an exit menu
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Exit")) {}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View"))
				{
					ImGui::Checkbox(ICON_FA_SATELLITE " Satellite list", &m_DrawSatelliteList);
					ImGui::Checkbox(ICON_FA_EARTH_EUROPE " 3D globe", &m_Draw3DGlobe);
					ImGui::Checkbox(ICON_FA_MAP " 2D map", &m_DrawWorldMap);
					ImGui::Checkbox(ICON_FA_CIRCLE " Polar view", &m_DrawPolarView);
					ImGui::Checkbox(ICON_FA_GLOBE " Sky at a glance" , &m_DrawSkyGlance);

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}
		}
		ImGui::End();

		if (m_DrawSatelliteList) DrawSatelliteList();
		if (m_Draw3DGlobe)       Draw3DViewport();
		if (m_DrawWorldMap)		 DrawMapViewport();
		if (m_DrawPolarView)	 DrawPolarView();
		if (m_DrawSkyGlance)	 DrawSkyGlance();

		rlImGuiEnd();

	EndDrawing();
}

void Application::Cleanup()
{
	UnloadRenderTexture(m_3DRenderTarget);
	rlImGuiEnd();
	CloseWindow();
}

void Application::LoadResources()
{
	m_EarthModel = LoadModel("res/Earth_1_12756.glb");
	m_SatelliteModel = LoadModel("res/satellite.glb");
	m_LocationBilboardMap = LoadTexture("res/location-bilboard-map.png");

	m_WorldMapTexture = LoadTexture("res/2k_earth_daymap.jpg");

	m_DroidSansFont = LoadFontEx("res/droid-sans/DroidSans-Bold.ttf", 32, NULL, 0);
}

void Application::ControlCamera()
{
	Matrix rotation = MatrixIdentity();

	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		rotation = MatrixRotate({ 0,1,0 }, -GetMouseDelta().x * 0.005);

		Vector3 right = Vector3CrossProduct(m_Camera.up, Vector3Subtract(m_Camera.position, m_Camera.target));
		right = Vector3Normalize(right);

		rotation = MatrixMultiply(rotation, MatrixRotate(right, -GetMouseDelta().y * 0.005));
	}

	Vector3 view = Vector3Subtract(m_Camera.position, m_Camera.target);
	
	view = Vector3Transform(view, rotation);
	m_Camera.position = Vector3Add(m_Camera.target, view);

	float zoom = GetMouseWheelMove() * 0.5f;
	if (zoom != 0)
	{
		float viewLen = Vector3Length(view);
		float newViewLen = Clamp(viewLen - zoom, 1.5, 100.0);

		view = Vector3Normalize(view);
		view = Vector3Scale(view, newViewLen);
		m_Camera.position = Vector3Add(m_Camera.target, view);
	}

}

void Application::SetupImGuiStyle()
{
	// Green Font style by aiekick from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(4.0f, 3.0f);
	style.FrameRounding = 0.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 0.0f;
	//style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 0.9399999976158142f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.4392156898975372f, 0.4392156898975372f, 0.4392156898975372f, 0.6000000238418579f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.5686274766921997f, 0.5686274766921997f, 0.5686274766921997f, 0.699999988079071f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.7568627595901489f, 0.7568627595901489f, 0.7568627595901489f, 0.800000011920929f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.03921568766236305f, 0.03921568766236305f, 0.03921568766236305f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.6000000238418579f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.5490196347236633f, 0.800000011920929f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.7490196228027344f, 0.800000011920929f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 1.0f, 0.800000011920929f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.5490196347236633f, 0.4000000059604645f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.7490196228027344f, 0.6000000238418579f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 1.0f, 0.800000011920929f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.5490196347236633f, 0.4000000059604645f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.7490196228027344f, 0.6000000238418579f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 1.0f, 0.800000011920929f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.5490196347236633f, 0.4000000059604645f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.7490196228027344f, 0.6000000238418579f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 1.0f, 0.800000011920929f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.5490196347236633f, 0.4000000059604645f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.7490196228027344f, 0.6000000238418579f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 1.0f, 0.800000011920929f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.5490196347236633f, 0.800000011920929f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 0.7490196228027344f, 0.800000011920929f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.1294117718935013f, 0.7490196228027344f, 1.0f, 0.800000011920929f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1764705926179886f, 0.1764705926179886f, 0.1764705926179886f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.3568627536296844f, 0.3568627536296844f, 0.3568627536296844f, 0.5400000214576721f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.07000000029802322f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}

bool Application::TrySelectSatellite(Satellite* satellite)
{
	if (std::find(m_SatelliteList.begin(), m_SatelliteList.end(), satellite) == m_SatelliteList.end()) return false;

	m_SelectedSatellite = satellite;
	// Also show it on the 3D explorer
	m_FollowSatellite = true;

	SetWindowTitle(std::string("SatHunter: " + m_SelectedSatellite->GetName()).c_str());
	return true;
}

void Application::Draw3DView()
{
	BeginTextureMode(m_3DRenderTarget);
	ClearBackground(BLACK);
	BeginMode3D(m_Camera);

	DrawGrid(50, 10);

	DrawModelEx(m_EarthModel, { 0, 0, 0 }, { 0, 1, 0 }, 180.0f, { 0.01, 0.01, 0.01 }, WHITE);

	Vector3 satPos = m_SelectedSatellite->GetPosition3D();

	if (IsKeyPressed(KEY_F1))
	{
		m_FollowSatellite = !m_FollowSatellite;
	}

	if (m_CanControlCamera)
	{
		m_Zoom -= GetMouseWheelMove() / 10;
		m_Zoom = Clamp(m_Zoom, 1.1f, 10.0f);
	}
	if (!m_FollowSatellite)
	{
		//UpdateCamera(&m_Camera, CAMERA_ORBITAL);
		if (m_CanControlCamera)
		{
			ControlCamera();
		}
	}
	else
	{
		m_Camera.position = {
			satPos.x * m_Zoom,
			satPos.y * m_Zoom,
			satPos.z * m_Zoom
		};
	}

	// Draw orbit
	int orbitIndex = 0;
	for (const auto& sat : m_SatelliteList)
	{
		DrawModelEx(m_SatelliteModel, sat->GetPosition3D(), {0, 0, 0}, 0.0f, {0.00015, 0.00015, 0.00015}, WHITE);

		Color orbitColor = OrbitColors[orbitIndex % 10];
		if (sat != m_SelectedSatellite)
		{
			orbitColor.a = 130;
		}

		for (int i = 0; i < 180 - 1; i++)
		{
			DrawLine3D(sat->GetOrbit().Points3D[i], sat->GetOrbit().Points3D[i + 1], orbitColor);
		}
		orbitIndex++;
	}

	// Draw ground station
	Vector3 groundStationECEF = ObserverToECEF(m_Config.Tracker.GroundStation);
	Vector3 groundStationPos = {
		groundStationECEF.y / 1260000, // Honestly don't know why the scale is so messed up
	0.1 + groundStationECEF.z / 1260000, // I would've guessed '1260' but apparently it's another x1000...
		groundStationECEF.x / 1260000  // Just works
	};

	//DrawSphere(groundStationPos, 0.2f, RED);
	DrawBillboard(m_Camera, m_LocationBilboardMap, groundStationPos, 0.5f, GREEN);

	EndMode3D();
	for (const auto& sat : m_SatelliteList)
	{
		Vector2 satLabelPos = GetWorldToScreenEx(sat->GetPosition3D(), m_Camera, m_3DRenderTarget.texture.width, m_3DRenderTarget.texture.height);
		
		if (sat == m_SelectedSatellite)
		{
			DrawTextEx(m_DroidSansFont, sat->GetName().c_str(), { satLabelPos.x - MeasureTextEx(m_DroidSansFont, sat->GetName().c_str(), 32, 1.0).x / 2.0f, satLabelPos.y }, 32, 1.0, GREEN);
		}
		else
		{
			DrawTextEx(m_DroidSansFont, sat->GetName().c_str(), { satLabelPos.x - MeasureTextEx(m_DroidSansFont, sat->GetName().c_str(), 16, 1.0).x / 2.0f, satLabelPos.y }, 16, 1.0, WHITE);
		}
	}

	Vector2 groundLabelPos = GetWorldToScreenEx(groundStationPos, m_Camera, m_3DRenderTarget.texture.width, m_3DRenderTarget.texture.height);
	DrawTextEx(m_DroidSansFont, m_Config.Tracker.GroundStationLabel.c_str(), { groundLabelPos.x - MeasureTextEx(m_DroidSansFont, m_Config.Tracker.GroundStationLabel.c_str(), 32, 1.0).x / 2.0f, groundLabelPos.y }, 32, 1.0, GREEN);

	EndTextureMode();
}

void Application::Draw3DViewport()
{
	ImGui::Begin("3D representation", &m_Draw3DGlobe);
	{
		if (m_3DRenderTarget.texture.width != ImGui::GetContentRegionAvail().x || m_3DRenderTarget.texture.height != ImGui::GetContentRegionAvail().y)
		{
			UnloadRenderTexture(m_3DRenderTarget);
			m_3DRenderTarget = LoadRenderTexture(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
		}

		m_CanControlCamera = false;
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
			&& ImGui::IsWindowHovered())
		{
			m_CanControlCamera = true;
		}

		rlImGuiImageRenderTexture(&m_3DRenderTarget);
	}
	ImGui::End();
}

void Application::DrawMap()
{
	BeginTextureMode(m_MapRenderTarget);
	ClearBackground(RAYWHITE);

	float texAspect = (float)m_WorldMapTexture.width / m_WorldMapTexture.height;
	float targetAspect = (float)m_MapRenderTarget.texture.width / m_MapRenderTarget.texture.height;

	float scale = (targetAspect > texAspect)
		? (float)m_MapRenderTarget.texture.height / m_WorldMapTexture.height
		: (float)m_MapRenderTarget.texture.width / m_WorldMapTexture.width;

	float drawWidth = m_WorldMapTexture.width * scale;
	float drawHeight = m_WorldMapTexture.height * scale;

	Rectangle dest = {
			(m_MapRenderTarget.texture.width - drawWidth) * 0.5f,
			(m_MapRenderTarget.texture.height - drawHeight) * 0.5f,
			drawWidth,
			drawHeight
	};

	DrawTexturePro(m_WorldMapTexture,
		{ 0, 0, (float)m_WorldMapTexture.width, (float)m_WorldMapTexture.height },
		dest, 
		{ 0, 0 },
		0.0f,
		WHITE
	);

	int orbitIndex = 0;
	for (const auto& sat : m_SatelliteList)
	{
		float orbitLength = 2.0f;
		Color orbitColor = OrbitColors[orbitIndex % 10];
		
		if (sat == m_SelectedSatellite)
		{
			orbitLength = 3.0f;
		}
		else
		{
			orbitColor.a = 180;
		}


		for (int i = 0; i < 180 - 1; i++)
		{
			Vector2 p1 = LatLonToRaylib(sat->GetOrbit().PointsGeodetic[i], m_MapRenderTarget.texture.width, m_MapRenderTarget.texture.height, dest);
			Vector2 p2 = LatLonToRaylib(sat->GetOrbit().PointsGeodetic[i + 1], m_MapRenderTarget.texture.width, m_MapRenderTarget.texture.height, dest);

			if (Vector2Distance(p1, p2) > 180.0f)
			{
				DrawCircleV(p1, orbitLength, orbitColor);
				continue;
			}

			DrawLineEx(p1, p2, orbitLength, orbitColor);
		}

		orbitIndex++;
	}

	// Draw the satellites on top of the orbits; that's why we loop 2 times
	orbitIndex = 0;
	for (const auto& sat : m_SatelliteList)
	{
		Vector2 satPos = LatLonToRaylib(sat->GetGeodetic(), m_MapRenderTarget.texture.width, m_MapRenderTarget.texture.height, dest);

		if (sat == m_SelectedSatellite)
		{
			DrawCircleV(satPos, 14.0f, BLACK);
		}

		DrawCircleV(satPos, 12.0f, OrbitColors[orbitIndex % 10]);
			
		Vector2 localMouse = Vector2Subtract(GetMousePosition(), m_MapWindowPosition);

		if (CheckCollisionPointCircle(localMouse, satPos, 12.0f) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			m_SelectedSatellite = sat;
			SetWindowTitle(std::string("SatHunter: " + m_SelectedSatellite->GetName()).c_str());
		}

		std::vector<libsgp4::CoordGeodetic> latLonCoveragePoints = sat->GetLatLonCoverage(m_MinElevationCoverage);
		for (int i = 0; i < 360 - 1; i++)
		{
			Vector2 p1 = LatLonToRaylib(latLonCoveragePoints[i], m_MapRenderTarget.texture.width, m_MapRenderTarget.texture.height, dest);
			Vector2 p2 = LatLonToRaylib(latLonCoveragePoints[i + 1], m_MapRenderTarget.texture.width, m_MapRenderTarget.texture.height, dest);

			// Skip them if too far from each other, will create werid rendering issues 
			if (Vector2Distance(p1, p2) > m_MapRenderTarget.texture.width / 2.0f)
			{
				DrawCircleV(p1, 0.5f, BLACK);
				DrawCircleV(p2, 0.5f, BLACK);
			}
			else
			{
				DrawLineEx(p1, p2, 1.0f, BLACK);
			}
		}

		DrawTextEx(m_DroidSansFont, sat->GetName().c_str(), { satPos.x - MeasureTextEx(m_DroidSansFont, sat->GetName().c_str(), 16, 1.0).x / 2.0f, satPos.y - 5 }, 16, 1.0, BLACK);
		orbitIndex++;
	}

	// Draw the ground station
	Vector2 groundStationPos = LatLonToRaylib(m_Config.Tracker.GroundStation.GetLocation(), m_MapRenderTarget.texture.width, m_MapRenderTarget.texture.height, dest);
	DrawTextureEx(m_LocationBilboardMap, { groundStationPos.x - m_LocationBilboardMap.width / 2 * 0.05f, groundStationPos.y - m_LocationBilboardMap.height * 0.05f }, 0.0f, 0.05f, RED);

	EndTextureMode();
}

void Application::DrawMapViewport()
{
	ImGui::Begin("World map", &m_DrawWorldMap);
	{
		if (m_MapRenderTarget.texture.width != ImGui::GetContentRegionAvail().x || m_MapRenderTarget.texture.height != ImGui::GetContentRegionAvail().y)
		{
			UnloadRenderTexture(m_MapRenderTarget);
			m_MapRenderTarget = LoadRenderTexture(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
		}

		m_MapWindowPosition = {
			ImGui::GetWindowPos().x + 8, // Adjust offset
			ImGui::GetWindowPos().y + 27
		};

		rlImGuiImageRenderTexture(&m_MapRenderTarget);
		
		ImVec2 sliderPos = { m_MapRenderTarget.texture.width / 2.0f - 200.0f, (float)m_MapRenderTarget.texture.height };

		ImGui::SetCursorScreenPos(sliderPos);
		ImGui::SetNextItemWidth(400);
		ImGui::SliderFloat("##SliderCoverage", &m_MinElevationCoverage, 0.0f, 25.0f, "Min Elev: %.1f");
	}
	ImGui::End();
}

void Application::DrawSatelliteList()
{
	ImGui::Begin("Satellite list", &m_DrawSatelliteList);
	{
		ImGui::BeginChild("All satellites", ImVec2(ImGui::GetContentRegionAvail().x/2, ImGui::GetContentRegionAvail().y));

		for (const auto& entry : m_TLEs)
		{
			const std::string& name = entry.first;
			const auto& tle = entry.second;

			if (ImGui::Selectable(name.c_str(), m_SelectedSatellite->GetName() == name))
			{
				// Check if it's already on the satellite list
				bool alreadyInTheList = false;
				for (auto& sat : m_SatelliteList)
				{
					if (sat->GetName() == name)
					{
						alreadyInTheList = true;
						TrySelectSatellite(sat);
						break;
					}
				}

				// If it's not in the list create a satellite for it 
				if (!alreadyInTheList)
				{
					Satellite* actualSatellite = new Satellite(entry.second);
					m_SatelliteList.push_back(actualSatellite);
					// Recompute the pass list
					m_PassData = PredictAllPasses(m_StartTime, m_StartTime.AddHours(24));
					TrySelectSatellite(actualSatellite);
				}
			}
		}

		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("Selection");

		for (int i = 0; i < m_SatelliteList.size();)
		{
			auto& sat = m_SatelliteList[i];
			ImGui::PushID(i);

			bool selected = m_SelectedSatellite == sat;

			if (ImGui::SmallButton(ICON_FA_ARROW_LEFT) && m_SatelliteList.size() > 1)
			{
				delete sat;
				m_SatelliteList.erase(i + m_SatelliteList.begin());
				if (selected) 
				{
					m_SelectedSatellite = m_SatelliteList[0];
					SetWindowTitle(std::string("SatHunter: " + m_SelectedSatellite->GetName()).c_str());
				}
				ImGui::PopID();
				// Recompute the pass list
				m_PassData = PredictAllPasses(m_StartTime, m_StartTime.AddHours(24));
				continue; // skip selectable, don't increment i
			}

			ImGui::SameLine();

			if (ImGui::Selectable(sat->GetName().c_str(), selected))
			{
				TrySelectSatellite(sat);
			}

			ImGui::PopID();
			++i;
		}

		ImGui::EndChild();
	}

	ImGui::End();
}

void Application::DrawPolarView()
{
	ImGui::Begin("Polar view", &m_DrawPolarView);
	{
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		ImVec2 canvas_pos = ImGui::GetCursorScreenPos();   // Top-left of drawing area
		ImVec2 canvas_size = ImGui::GetContentRegionAvail(); // Size of drawing area
		float radius = std::min(canvas_size.x, canvas_size.y) * 0.5f;

		ImVec2 center = ImVec2(canvas_pos.x + canvas_size.x * 0.5f,
			canvas_pos.y + canvas_size.y * 0.5f);

		for (int i = 1; i <= 3; ++i)
		{
			float r = radius * (i / 3.0f);
			draw_list->AddCircle(center, r, IM_COL32(100, 100, 100, 100), 64, 1.0f);
		}

		draw_list->AddLine(ImVec2(center.x - radius, center.y), ImVec2(center.x + radius, center.y),
			IM_COL32(150, 150, 150, 100));
		draw_list->AddLine(ImVec2(center.x, center.y - radius), ImVec2(center.x, center.y + radius),
			IM_COL32(150, 150, 150, 100));

		draw_list->AddText(ImVec2(center.x, center.y - radius), IM_COL32_WHITE, "N");
		draw_list->AddText(ImVec2(center.x + radius - 10, center.y), IM_COL32_WHITE, "E");
		draw_list->AddText(ImVec2(center.x, center.y + radius - 15), IM_COL32_WHITE, "S");
		draw_list->AddText(ImVec2(center.x - radius, center.y), IM_COL32_WHITE, "W");

		if (ImGui::IsWindowHovered())
		{
			ImVec2 mouse = ImGui::GetMousePos();
			ImVec2 diff = ImVec2(mouse.x - center.x, mouse.y - center.y);
			float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

			float norm = std::min(dist / radius, 1.0f);

			float elevation = (1.0f - norm) * 90.0f;

			float azimuth_rad = std::atan2(diff.x, -diff.y);
			float azimuth_deg = azimuth_rad * (180.0f / PI);
			if (azimuth_deg < 0) azimuth_deg += 360.0f;

			ImGui::SetTooltip("Azimuth: %.3f, Elevation: %.3f", azimuth_deg, elevation);
		}

		libsgp4::CoordTopocentric topo = m_Config.Tracker.GroundStation.GetLookAngle(m_SelectedSatellite->GetEci());
		auto TopoToXY = [&](libsgp4::CoordTopocentric coordTopo) -> ImVec2
		{
			float azimuth_rad = coordTopo.azimuth;  // Azimuth in radians
			float elevation_deg = coordTopo.elevation * RAD2DEG; // Elevation in degrees

			float norm = 1 - elevation_deg / 90.0f;

			float x = center.x + radius * sinf(azimuth_rad) * norm;
			float y = center.y - radius * cosf(azimuth_rad) * norm;

			return ImVec2(x, y);
		};
		if (m_PassData.find(m_SelectedSatellite->GetName()) != m_PassData.end())
		{
			const auto& passes = m_PassData[m_SelectedSatellite->GetName()];
			if (!passes.empty())
			{
				const PassDetails& currentPass = passes[0];
				if (currentPass.AOS.Ticks() >= libsgp4::DateTime::Now().Ticks()
				&&  currentPass.LOS.Ticks() <= libsgp4::DateTime::Now().Ticks())
				{
					if (!currentPass.PointsTopocentric.empty())
					{
						for (int i = 0; i < currentPass.PointsTopocentric.size() - 1; i++)
						{
							draw_list->AddLine(
								TopoToXY(currentPass.PointsTopocentric[i]),
								TopoToXY(currentPass.PointsTopocentric[i + 1]),
								IM_COL32(255, 255, 255, 255)
							);
						}
					}
				}
			}
		}

		if (topo.elevation > 0)
		{
			// Draw it anyways; maybe it's geostationary
			draw_list->AddCircleFilled(TopoToXY(topo), 5.0f, IM_COL32(255, 0, 0, 255)); // Red point, size 5
		}
	}
	ImGui::End();
}

float DateTimeToSeconds(const libsgp4::DateTime& dt)
{
	int year = dt.Year();
	int month = dt.Month();
	int day = dt.Day();
	int hour = dt.Hour();
	int minute = dt.Minute();
	int second = dt.Second();

	struct tm tm = {};
	tm.tm_year = year - 1900; // tm_year is years since 1900
	tm.tm_mon = month - 1;    // tm_mon is months since January (0-11)
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = minute;
	tm.tm_sec = second;

	time_t raw_time = mktime(&tm);
	return static_cast<float>(raw_time);
}

void Application::DrawSkyGlance()
{
	libsgp4::TimeSpan span(24, 0, 0);

	float rowHeight = 20.0f;
	float rowSpacing = 5.0f;
	float textOffset = 5.0f;

	ImGui::Begin("Sky at a glance", &m_DrawSkyGlance);
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		canvasPos.x += 100;
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();
		canvasSize.x -= 100;
		ImVec2 canvasEnd = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);

		// Draw border
		drawList->AddRect(canvasPos, canvasEnd, IM_COL32(255, 255, 255, 120));

		int row = 0;
		PassDetails* nextPass = nullptr;
		std::string nextPassSatName = "";

		for (auto& [satName, passes] : m_PassData)
		{
			float y = canvasPos.y + row * (rowHeight + rowSpacing);

			drawList->AddText(ImVec2(canvasPos.x - 95, y + textOffset), IM_COL32_WHITE, satName.c_str());

			for (auto& pass : passes)
			{
				libsgp4::TimeSpan AOS(pass.AOS.Ticks() - libsgp4::DateTime::Now().Ticks());
				libsgp4::TimeSpan LOS(pass.LOS.Ticks() - libsgp4::DateTime::Now().Ticks());
				double AOSTimeSeconds = AOS.TotalSeconds();
				double LOSTimeSeconds = LOS.TotalSeconds();

				double totalSpanSeconds = span.TotalSeconds();

				float x0 = canvasPos.x + (float)(AOSTimeSeconds / totalSpanSeconds) * canvasSize.x;
				float x1 = canvasPos.x + (float)(LOSTimeSeconds / totalSpanSeconds) * canvasSize.x;

				drawList->AddRectFilled(
					ImVec2(x0, y),
					ImVec2(x1, y + rowHeight),
					IM_COL32(100, 200, 255, 200)
				);

				if (!nextPass || pass.AOS < nextPass->AOS)
				{
					nextPass = &pass;
					nextPassSatName = satName;
				}
			}

			row++;
		}

		// Draw next pass
		if (nextPass)
		{
			ImGui::SetCursorPosY(canvasSize.y);
		
			auto offset = std::chrono::current_zone()->get_info(std::chrono::system_clock::now()).offset;

			int hours = nextPass->AOS.AddSeconds(offset.count()).Hour();
			int minutes = nextPass->AOS.Minute();
			int seconds = nextPass->AOS.Second();

			ImGui::Text("Next pass: %s at %02d:%02d:%02d", nextPassSatName.c_str(), hours, minutes, seconds);
		}

		if (ImGui::IsWindowHovered())
		{
			ImVec2 mousePos = ImGui::GetMousePos();
			if (mousePos.x >= canvasPos.x && mousePos.x <= canvasEnd.x)
			{
				// Calculate the time corresponding to the x-coordinate
				float mouseXRelative = mousePos.x - canvasPos.x;  // Mouse position relative to canvas
				double timeAtX = (mouseXRelative / canvasSize.x) * span.TotalSeconds(); // Map x to time in seconds

				// Convert seconds to a time format (e.g., hours and minutes)
				int hours = static_cast<int>(timeAtX) / 3600;
				int minutes = (static_cast<int>(timeAtX) % 3600) / 60;
				int seconds = static_cast<int>(timeAtX) % 60;

				// Format time string
				char timeStr[64];
				snprintf(timeStr, sizeof(timeStr), "Time: +%02d:%02d:%02d", hours, minutes, seconds);

				// Show tooltip with the time
				ImGui::SetTooltip(timeStr);
			}
		}
	}
	ImGui::End();
}

std::vector<Application::PassDetails> Application::PredictPass(Satellite* sat, const libsgp4::DateTime& start, const libsgp4::DateTime& end, float minElevation)
{
	std::vector<PassDetails> passes;
	PassDetails currentPass;
	bool inPass = false;
	bool losMarked = false;
	libsgp4::DateTime currentTime = start;

	while (currentTime <= end)
	{
		libsgp4::CoordTopocentric topo = m_Config.Tracker.GroundStation.GetLookAngle(sat->GetEciTimed(currentTime));
		float elevationDeg = topo.elevation * RAD2DEG;

		if (elevationDeg >= 0)
			currentPass.PointsTopocentric.push_back(topo);

		if (elevationDeg >= minElevation)
		{
			if (!inPass)
			{
				// Just entered a pass
				inPass = true;
				currentPass.AOS = currentTime;
				currentPass.MaxElevation = elevationDeg; // Set it for now to the elevation
			}
			else
			{
				// We are already in the middle of a pass
				if (elevationDeg > currentPass.MaxElevation) currentPass.MaxElevation = elevationDeg;
			}
		}
		else if (inPass)
		{
			// Just exited this pass
			inPass = false;
			currentPass.LOS = currentTime;
			passes.push_back(currentPass);

			// Reset for the next pass
			currentPass = PassDetails();
		}

		currentTime = currentTime + libsgp4::TimeSpan(0, 0, 10);
	}

	return passes;
}

Application::PassData Application::PredictAllPasses(const libsgp4::DateTime& start, const libsgp4::DateTime& end, float minElevation)
{
	PassData passData;

	for (auto& sat : m_SatelliteList)
	{
		auto passes = PredictPass(sat, start, end, minElevation);

		if (!passes.empty())
			passData[sat->GetName()] = std::move(passes);
	}

	return passData;
}

}