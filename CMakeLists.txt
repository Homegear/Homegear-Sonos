cmake_minimum_required(VERSION 3.8)
project(homegear_sonos)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        src/PhysicalInterfaces/EventServer.cpp
        src/PhysicalInterfaces/EventServer.h
        src/PhysicalInterfaces/ISonosInterface.cpp
        src/PhysicalInterfaces/ISonosInterface.h
        src/Factory.cpp
        src/Factory.h
        src/GD.cpp
        src/GD.h
        src/Interfaces.cpp
        src/Interfaces.h
        src/Sonos.cpp
        src/Sonos.h
        src/SonosCentral.cpp
        src/SonosCentral.h
        src/SonosPacket.cpp
        src/SonosPacket.h
        src/SonosPeer.cpp
        src/SonosPeer.h
        config.h)

add_custom_target(homegear COMMAND ../../makeAll.sh SOURCES ${SOURCE_FILES})

add_library(homegear_sonos ${SOURCE_FILES})