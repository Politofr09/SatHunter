#pragma once

#include <string>
#include <unordered_map>
#include <SGP4.h>

namespace SatHunter {

#define TLE_FILE "weather.txt"

// Download TLE data and store it in weather.txt file exists
bool CheckTLEs(const std::string url);

std::unordered_map<std::string, libsgp4::Tle> LoadTLEs();

#ifdef EMSCRIPTEN
std::unordered_map<std::string, libsgp4::Tle> GetTLEsForWeb();
#endif

}