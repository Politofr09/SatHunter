#pragma once

#include <string>
#include <Observer.h>

#define INI_CONFIG_FILE "config.ini"

namespace SatHunter {

struct Config
{
	struct _General
	{
		std::string TleUrl = "https://celestrak.org/norad/elements/weather.txt";
	} General;

	struct _Tracker
	{
		std::string Satellite = "NOAA 15";
		libsgp4::Observer GroundStation = libsgp4::Observer(0, 0, 0);
		std::string GroundStationLabel = "Ground station";
	} Tracker;
};

Config LoadConfigFile();

}