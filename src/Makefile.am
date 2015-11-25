AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear

libdir = $(localstatedir)/lib/homegear/modules
lib_LTLIBRARIES = mod_sonos.la
mod_sonos_la_SOURCES = SonosPacket.cpp Sonos.cpp Factory.cpp GD.h Interfaces.h Interfaces.cpp SonosPeer.cpp SonosPacket.h SonosPeer.h Sonos.h GD.cpp Factory.h PhysicalInterfaces/ISonosInterface.h PhysicalInterfaces/EventServer.h PhysicalInterfaces/ISonosInterface.cpp PhysicalInterfaces/EventServer.cpp SonosCentral.h SonosCentral.cpp
mod_sonos_la_LDFLAGS =-module -avoid-version -shared
install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/mod_sonos.la
