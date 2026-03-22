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

	std::string TLEList = configFile["General"]["tle_urls"].as<const char*>();
	std::stringstream ss(TLEList);
	std::string buffer;

	while (std::getline(ss, buffer, ',')) {
		conf.General.TleUrls.push_back(Trim(buffer));
	}

	std::string satList = configFile["Tracker"]["satellites"].as<const char*>();

	ss = std::stringstream(satList);
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

void SerializeConfigFile(Config& conf)
{
	// // Right now I will just update the conf.General.TleUrl field
	// // Reserializing everything with inicpp would mean also deleting the comments
	// const std::string& path = "res/" + std::string(INI_CONFIG_FILE);
	// std::ifstream inFile(path);
    // std::stringstream buffer;
    // std::string line;

    // bool inGeneralSection = false;

    // while (std::getline(inFile, line))
    // {
    //     std::string trimmed = line;

	// 	if (trimmed.find(";") != std::string::npos)
    // 		continue;
    //     // Detect section
    //     if (trimmed.find("[General]") != std::string::npos)
    //     {
    //         inGeneralSection = true;
    //     }
    //     else if (!trimmed.empty() && trimmed[0] == '[')
    //     {
    //         inGeneralSection = false;
    //     }

    //     // Replace tle_url inside [General]
    //     if (inGeneralSection && trimmed.find("tle_url") != std::string::npos)
    //     {
    //         line = "tle_url = " + conf.General.TleUrl;
    //     }

    //     buffer << line << "\n";
    // }

    // inFile.close();

    // std::ofstream outFile(path);
    // outFile << buffer.str();
	return;
}

}