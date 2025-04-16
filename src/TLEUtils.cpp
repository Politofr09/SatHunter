#include "TLEUtils.h"

#include <cpr/cpr.h>

#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <format>
#include <iostream>
#include <string>

namespace SatHunter {
	
// Checks if weather.txt exists and weather.txt.timestamp is less than 24h old
bool CheckTimestampFile()
{
	if (!std::filesystem::exists(TLE_FILE))
	{
		return false;
	}

	std::ifstream tsFile(std::string(TLE_FILE) + ".timestamp");
	if (!tsFile) return false;

	std::stringstream buffer;
	buffer << tsFile.rdbuf();
	std::istringstream data(buffer.str());

	std::chrono::sys_time<std::chrono::milliseconds> timestamp;
	data >> std::chrono::parse("%FT%T%Ez", timestamp);
	if (data.fail())
		return false;

	auto now = std::chrono::system_clock::now();

	auto duration = now - timestamp;

	return duration >= std::chrono::hours(24);
}

bool CheckTLEs(const std::string url)
{
	if (CheckTimestampFile())
	{
		std::cout << "Using cached TLE data" << std::endl;
		return false;
	}

	std::cout << "TLE data is outdated. Fetching new TLE data... ";

	// We need to update weather.txt and weather.txt.timestamp
	cpr::Response r = cpr::Get(cpr::Url{ url });

	if (r.status_code != 200)
	{
		std::cerr << "\nFailed to fetch TLE data: HTTP " << r.status_code << std::endl;
		return false;
	}

	std::cout << "Done." << std::endl;

	// Write it to txt file
	std::ofstream tleFile(TLE_FILE, std::ios::binary);
	tleFile << r.text;
	tleFile.close();

	// Get current time (UTC) and write to .timestamp
	auto now = std::chrono::system_clock::now();
	std::ofstream tsFile(std::string(TLE_FILE) + ".timestamp");
	tsFile << std::format("{:%FT%T%Ez}", now);
	tsFile.close();

	std::cout << "TLE data updated and timestamp saved. TLE local file will expire in 24 hours." << std::endl;
	return true;
}

std::unordered_map<std::string, libsgp4::Tle> LoadTLEs()
{
	std::unordered_map<std::string, libsgp4::Tle> tles;

	std::ifstream file(TLE_FILE);
	if (!file.is_open())
	{
		std::cerr << "Failed to open TLE file.\n";
		return {};
	}

	std::string satName, line1, line2;
	while (std::getline(file, satName))
	{
		// Remove trailing spaces from satName
		satName.erase(std::find_if(satName.rbegin(), satName.rend(), [](unsigned char ch) {
			return !std::isspace(ch);  // Find the first non-space character from the end
		}).base(), satName.end());

		if (!std::getline(file, line1)) break;
		if (!std::getline(file, line2)) break;

		try {
			#ifdef __linux__
			line1.pop_back();
			line2.pop_back();
			#endif

			libsgp4::Tle tle(satName, line1, line2);
			tles.emplace(satName, std::move(tle));
		}
		catch (const std::exception& e) {
			std::cerr << "Failed to parse TLE: " << e.what() << "\n";
		}
	}

	return tles;
}

#ifdef EMSCRIPTEN
#include <emscripten/fetch.h>
std::unordered_map<std::string, libsgp4::Tle> tleMap;

void downloadSucceeded(emscripten_fetch_t* fetch) 
{
	std::istringstream ss(std::string(fetch->data, fetch->numBytes));
	std::string line1, line2, name;

	while (std::getline(ss, name) && std::getline(ss, line1) && std::getline(ss, line2))
	{
		try 
		{
			line1.pop_back();
			line2.pop_back();
			tleMap.emplace(name, libsgp4::Tle(name, line1, line2));
		}
		catch (...) 
		{
			// Handle malformed TLEs gracefully
		}
	}

	emscripten_fetch_close(fetch);
}

void downloadFailed(emscripten_fetch_t* fetch) 
{
	printf("Failed to fetch TLEs from %s\n", fetch->url);
	emscripten_fetch_close(fetch);
}

std::unordered_map<std::string, libsgp4::Tle>& GetTLEForWeb(const& std::string url)
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;

		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);
		strcpy(attr.requestMethod, "GET");
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
		attr.onsuccess = downloadSucceeded;
		attr.onerror = downloadFailed;

		emscripten_fetch(&attr, url.c_str());
	}

	return tleMap;
}
#endif

}