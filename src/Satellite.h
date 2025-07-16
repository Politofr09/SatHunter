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

class Satellite
{
public:
	Satellite();
	Satellite(libsgp4::Tle tle);

	Satellite& operator=(const Satellite& other)
	{
		m_Name = other.m_Name;
		m_OrbitPath.Points3D = other.m_OrbitPath.Points3D;
		m_OrbitPath.PointsGeodetic = other.m_OrbitPath.PointsGeodetic;
		m_TLE = other.m_TLE;
		m_SGP4Instance.SetTle(m_TLE);
		return *this;
	}

	bool operator==(const Satellite& rhs)
	{
		return m_Name == rhs.m_Name;
	}

	const std::string& GetName() const { return m_Name; }
	const OrbitPath& GetOrbit() const { return m_OrbitPath; }

	// Refresh the orbit every 10 mins or so by calling this
	void RecalculateOrbit();

	Vector3 GetPosition3D() const;
	libsgp4::CoordGeodetic GetGeodetic() const;
	libsgp4::Eci GetEci() const;
	libsgp4::Eci GetEciTimed(const libsgp4::DateTime dt) const;

	// Returns a vector of 360 Lat/Lon points
	std::vector<libsgp4::CoordGeodetic> GetLatLonCoverage(float minElevation) const;

private:
	libsgp4::SGP4 m_SGP4Instance;
	libsgp4::Tle  m_TLE;
	std::string   m_Name;
	OrbitPath	  m_OrbitPath;
};

Vector3 GetSatellitePosition(libsgp4::SGP4 sgp4, libsgp4::DateTime dt);
OrbitPath GetOrbitPath(libsgp4::SGP4 sgp4);

Vector3 ObserverToECEF(libsgp4::Observer o);

// Map lat long to raylib coordinates. Display it in a standard equirectangular world map.
Vector2 LatLonToRaylib(const libsgp4::CoordGeodetic& geo, float mapWidth, float mapHeight, Rectangle dest);

}