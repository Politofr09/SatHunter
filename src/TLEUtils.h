#pragma once

#include <string>
#include <unordered_map>
#include <SGP4.h>

namespace SatHunter {

// Download TLE data from a list of urls and stores each url in a hashed filename
// Then creates a timestamp file to know when was the last time we fetched the TLEs
bool CheckTLEs(const std::vector<std::string>& urls);

std::unordered_map<std::string, libsgp4::Tle> LoadTLEs(const std::vector<std::string>& urls);

}