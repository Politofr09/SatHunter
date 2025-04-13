#include "Satellite.h"
#include <raymath.h>

namespace SatHunter {

Vector3 GetSatellitePosition(libsgp4::SGP4 sgp4, libsgp4::DateTime dt)
{
	libsgp4::Eci satPosECI = sgp4.FindPosition(dt);

	double gmst = dt.ToGreenwichSiderealTime();

	Vector3 satPosECEF;
	satPosECEF.x = satPosECI.Position().x * cos(gmst) + satPosECI.Position().y * sin(gmst);
	satPosECEF.y = satPosECI.Position().x * -sin(gmst) + satPosECI.Position().y * cos(gmst);
	satPosECEF.z = satPosECI.Position().z;

	return {
		satPosECEF.y / 1300,
		satPosECEF.z / 1300,
		satPosECEF.x / 1300
	};
}

OrbitPath GetOrbitPath(libsgp4::SGP4 sgp4)
{
	libsgp4::DateTime start = libsgp4::DateTime::Now().AddMinutes(-10);

	OrbitPath orbit;
	orbit.Points3D.resize(180);
	orbit.PointsGeodetic.resize(180);
	for (int i = 0; i < 180; i++)
	{
		libsgp4::DateTime dt = start.AddMinutes(i);
		orbit.Points3D[i] = GetSatellitePosition(sgp4, dt);

		orbit.PointsGeodetic[i] = sgp4.FindPosition(dt).ToGeodetic();
	}

	return orbit;
}

Vector3 ObserverToECEF(libsgp4::Observer o)
{
	// Convert degrees to radians
	double lat = o.GetLocation().latitude;
	double lon = o.GetLocation().longitude;
	double alt = o.GetLocation().altitude;

	// WGS84 constants
	constexpr double a = 6378137.0;             // Equatorial radius (m)
	constexpr double e2 = 6.69437999014e-3;     // Eccentricity squared

	// Prime vertical radius of curvature
	double N = a / std::sqrt(1.0 - e2 * std::sin(lat) * std::sin(lat));

	// Calculate ECEF coordinates
	float x = (N + alt) * std::cos(lat) * std::cos(lon);
	float y = (N + alt) * std::cos(lat) * std::sin(lon);
	float z = (N * (1 - e2) + alt) * std::sin(lat);

	return { x, y, z };
}

Vector2 LatLonToRaylib(const libsgp4::CoordGeodetic& geo, float mapWidth, float mapHeight, Rectangle dest)
{
	float lat = geo.latitude * RAD2DEG;
	float lon = geo.longitude * RAD2DEG;

	// Map [-180, 180] lon to [0, 1]
	float xNorm = (lon + 180.0f) / 360.0f;
	// Map [90, -90] lat to [0, 1] (flip lat)
	float yNorm = (90.0f - lat) / 180.0f;

	// Scale to destination rectangle
	return {
		dest.x + xNorm * dest.width,
		dest.y + yNorm * dest.height
	};
}

}