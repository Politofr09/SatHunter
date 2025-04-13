#include "Config.h"

#include <inicpp.h>

namespace SatHunter {

Config LoadConfigFile()
{
	ini::IniFile configFile;
	configFile.allowOverwriteDuplicateFields(true);
	configFile.load("res/" + std::string(INI_CONFIG_FILE));

	Config conf{};

	conf.General.TleUrl = configFile["General"]["tle_url"].as<const char*>();

	conf.Tracker.Satellite = configFile["Tracker"]["satellite"].as<const char*>();

	std::string groundStationStr = configFile["Tracker"]["ground_station"].as<const char*>();

	float lat = 0.0f, lon = 0.0f;
	{
		std::stringstream ss(groundStationStr);
		std::string latStr, lonStr;

		if (std::getline(ss, latStr, ',') && std::getline(ss, lonStr, ',')) {
			lat = std::stof(latStr);
			lon = std::stof(lonStr);
		}
		else {
			std::cerr << "Failed to parse ground_station coordinates.\n";
		}
	}

	conf.Tracker.GroundStation = libsgp4::Observer(lat, lon, 0.0);

	conf.Tracker.GroundStationLabel = configFile["Tracker"]["ground_station_label"].as<const char*>();

	return conf;
}

}