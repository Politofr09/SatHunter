#pragma once

#include <raylib.h>
#include <SGP4.h>
#include <Observer.h>
#include <chrono>

#define SATELLITE_ORBIT_SCALE 1000

namespace SatHunter {

struct OrbitPath
{
	std::vector<Vector3> Points3D;
	std::vector<libsgp4::CoordGeodetic> PointsGeodetic;
};

Vector3 GetSatellitePosition(libsgp4::SGP4 sgp4, libsgp4::DateTime dt);
OrbitPath GetOrbitPath(libsgp4::SGP4 sgp4);

Vector3 ObserverToECEF(libsgp4::Observer o);

// Map lat long to raylib coordinates. Display it in a standard equirectangular world map.
Vector2 LatLonToRaylib(const libsgp4::CoordGeodetic& geo, float mapWidth, float mapHeight, Rectangle dest);

}