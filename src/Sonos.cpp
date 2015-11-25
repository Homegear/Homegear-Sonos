/* Copyright 2013-2015 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "Sonos.h"
#include "SonosCentral.h"
#include "Interfaces.h"
#include "GD.h"

namespace Sonos
{

Sonos::Sonos(BaseLib::Obj* bl, BaseLib::Systems::DeviceFamily::IFamilyEventSink* eventHandler) : BaseLib::Systems::DeviceFamily(bl, eventHandler, SONOS_FAMILY_ID, "Sonos")
{
	GD::bl = bl;
	GD::family = this;
	GD::settings = _settings;
	GD::out.init(bl);
	GD::out.setPrefix("Module Sonos: ");
	GD::out.printDebug("Debug: Loading module...");
	GD::rpcDevices.init(_bl, this);
	_physicalInterfaces.reset(new Interfaces(bl, _settings->getPhysicalInterfaceSettings()));
}

Sonos::~Sonos()
{

}

bool Sonos::init()
{
	GD::out.printInfo("Loading XML RPC devices...");
	GD::rpcDevices.load();
	if(GD::rpcDevices.empty()) return false;
	return true;
}

void Sonos::dispose()
{
	if(_disposed) return;
	DeviceFamily::dispose();

	_central.reset();
	GD::rpcDevices.clear();
}

std::shared_ptr<BaseLib::Systems::ICentral> Sonos::getCentral() { return _central; }

void Sonos::load()
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows = _bl->db->getDevices((uint32_t)getFamily());
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			uint32_t deviceID = row->second.at(0)->intValue;
			GD::out.printMessage("Loading Sonos device " + std::to_string(deviceID));
			std::string serialNumber = row->second.at(2)->textValue;
			uint32_t deviceType = row->second.at(3)->intValue;

			if(deviceType == 0xFFFFFFFD)
			{
				_central = std::shared_ptr<SonosCentral>(new SonosCentral(deviceID, serialNumber, this));
				_central->load();
				_central->loadPeers();
			}
		}
		if(!_central) createCentral();
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void Sonos::createCentral()
{
	try
	{
		if(_central) return;

		int32_t seedNumber = BaseLib::HelperFunctions::getRandomNumber(1, 9999999);
		std::ostringstream stringstream;
		stringstream << "VSC" << std::setw(7) << std::setfill('0') << std::dec << seedNumber;
		std::string serialNumber(stringstream.str());

		_central.reset(new SonosCentral(0, serialNumber, this));
		GD::out.printMessage("Created Sonos central with id " + std::to_string(_central->getId()) + " and serial number " + serialNumber);
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string Sonos::handleCliCommand(std::string& command)
{
	try
	{
		std::ostringstream stringStream;
		if(!_central) return "Error: No central exists.\n";
		return _central->handleCliCommand(command);
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "Error executing command. See log file for more details.\n";
}

PVariable Sonos::getPairingMethods()
{
	try
	{
		if(!_central) return PVariable(new Variable(VariableType::tArray));
		PVariable array(new Variable(VariableType::tArray));
		array->arrayValue->push_back(PVariable(new Variable(std::string("searchDevices"))));
		return array;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return Variable::createError(-32500, "Unknown application error.");
}
}
