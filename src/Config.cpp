#include "Config.h"

#include <inicpp.h>

namespace SatHunter {

inline static std::string Trim(const std::string& str) {
	const auto start = str.find_first_not_of(" \t\n\r");
	const auto end = str.find_last_not_of(" \t\n\r");
	return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

Config LoadConfigFile()
{
	ini::IniFile configFile;
	configFile.allowOverwriteDuplicateFields(true);
	configFile.load("res/" + std::string(INI_CONFIG_FILE));

	Config conf{};

	conf.General.TleUrl = configFile["General"]["tle_url"].as<const char*>();

	std::string satList = configFile["Tracker"]["satellites"].as<const char*>();

	std::stringstream ss(satList);
	std::string buffer;

	while (std::getline(ss, buffer, ',')) {
		conf.Tracker.Satellites.push_back(Trim(buffer));
	}

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