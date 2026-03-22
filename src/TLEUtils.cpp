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

// We can't just store the url because the file system will complain about the symbols.
// Assigns a unique hash filename to a given url.
std::string UrlHashFilename(const std::string& url) 
{
    std::hash<std::string> hasher;
    size_t h = hasher(url);
    return "TLE_" + std::to_string(h) + ".txt";
}

// Checks if all urls are stored and checks if the timestamp.ts file has expired
bool CheckTimestampFile(const std::vector<std::string>& urls)
{
	
	for (const auto& url : urls)
	{
		if (!std::filesystem::exists(UrlHashFilename(url)))
		{
			return false;
		}
	}

    std::ifstream tsFile("timestamp.ts");
    if (!tsFile) return false;

    std::string line;
    std::getline(tsFile, line);
    if (line.size() < 16) return false;  // Needs at least "YYYY-MM-DDTHH:MM"

    std::string truncated = line.substr(0, 16);

    std::tm tm = {};
    std::istringstream ss(truncated);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M");
    if (ss.fail()) return false;

#if defined(_WIN32)
    std::time_t tt = _mkgmtime(&tm); // Windows: treat as UTC
#else
    std::time_t tt = timegm(&tm);    // Linux: treat as UTC
#endif

    auto timestamp = std::chrono::system_clock::from_time_t(tt);
    auto now = std::chrono::system_clock::now();

    auto duration = now - timestamp;

    return duration < std::chrono::hours(24); // Check if *less* than 24h
}

bool CheckTLEs(const std::vector<std::string>& urls)
{
    if (CheckTimestampFile(urls))
    {
        std::cout << "Using cached TLE data" << std::endl;
        return false;
    }

    std::cout << "TLE sources are outdated or missing. Fetching new TLE data...\n";

    for (const auto& url : urls)
    {
        std::cout << "Fetching: " << url << std::endl;

        cpr::Response r = cpr::Get(cpr::Url{ url });

        if (r.status_code != 200)
        {
            std::cerr << "Failed: HTTP " << r.status_code << std::endl;
            continue;
        }

        std::ofstream tleFile(UrlHashFilename(url), std::ios::binary);
        tleFile << r.text;
    }

    // Update global timestamp
    auto now = std::chrono::system_clock::now();
    std::ofstream tsFile("timestamp.ts");
    tsFile << std::format("{:%FT%T%Ez}", now);

    std::cout << "TLE sources updated. Cache valid for 24 hours.\n";

    return true;
}

std::unordered_map<std::string, libsgp4::Tle> LoadTLEs(const std::vector<std::string>& urls)
{
    std::unordered_map<std::string, libsgp4::Tle> tles;

    for (const auto& url : urls)
    {
        std::ifstream file(UrlHashFilename(url));
        if (!file.is_open())
        {
            std::cerr << "Failed to open TLE file for URL: " << url << "\n";
            continue;
        }

        std::string satName, line1, line2;

        while (std::getline(file, satName))
        {
            // Trim spaces
            satName.erase(std::find_if(satName.rbegin(), satName.rend(),
                [](unsigned char ch) { return !std::isspace(ch); }).base(),
                satName.end());

            if (!std::getline(file, line1)) break;
            if (!std::getline(file, line2)) break;

            try {
			#ifdef __linux__
                if (!line1.empty()) line1.pop_back();
                if (!line2.empty()) line2.pop_back();
			#endif

                libsgp4::Tle tle(satName, line1, line2);

                // Deduplication happens here:
                tles[satName] = std::move(tle);
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to parse TLE: " << e.what() << "\n";
            }
        }
    }

    return tles;
}

}