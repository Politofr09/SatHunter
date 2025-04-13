cmake_minimum_required(VERSION 3.22)
project(sgp4)

set(SRCS
    CoordGeodetic.cc
    CoordTopocentric.cc
    DateTime.cc
    DecayedException.cc
    Eci.cc
    Globals.cc
    Observer.cc
    OrbitalElements.cc
    SGP4.cc
    SatelliteException.cc
    SolarPosition.cc
    TimeSpan.cc
    Tle.cc
    TleException.cc
    Util.cc
    Vector.cc)

  set(INCS
     CoordGeodetic.h
     CoordTopocentric.h
     DateTime.h
     DecayedException.h
     Eci.h
     Globals.h
     Observer.h
     OrbitalElements.h
     SatelliteException.h
     SGP4.h
     SolarPosition.h
     TimeSpan.h
     TleException.h
     Tle.h
     Util.h
     Vector.h
     )

add_library(sgp4 STATIC ${SRCS} ${INCS})

# Set include directories for the library so other projects can use it
target_include_directories(sgp4 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")