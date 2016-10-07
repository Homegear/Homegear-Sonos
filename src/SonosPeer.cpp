/* Copyright 2013-2016 Sathya Laufer
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

#include "SonosPeer.h"
#include "SonosCentral.h"
#include "SonosPacket.h"
#include "GD.h"
#include "sys/wait.h"

namespace Sonos
{
std::shared_ptr<BaseLib::Systems::ICentral> SonosPeer::getCentral()
{
	try
	{
		if(_central) return _central;
		_central = GD::family->getCentral();
		return _central;
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
	return std::shared_ptr<BaseLib::Systems::ICentral>();
}

SonosPeer::SonosPeer(uint32_t parentID, IPeerEventSink* eventHandler) : BaseLib::Systems::Peer(GD::bl, parentID, eventHandler)
{
	init();
}

SonosPeer::SonosPeer(int32_t id, std::string serialNumber, uint32_t parentID, IPeerEventSink* eventHandler) : BaseLib::Systems::Peer(GD::bl, id, -1, serialNumber, parentID, eventHandler)
{
	init();
}

SonosPeer::~SonosPeer()
{
	try
	{

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

void SonosPeer::init()
{
	_binaryEncoder.reset(new BaseLib::Rpc::RpcEncoder(GD::bl));
	_binaryDecoder.reset(new BaseLib::Rpc::RpcDecoder(GD::bl));

	_upnpFunctions.insert(UpnpFunctionPair("AddURIToQueue", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues()))));
	_upnpFunctions.insert(UpnpFunctionPair("Browse", UpnpFunctionEntry("urn:schemas-upnp-org:service:ContentDirectory:1", "/MediaServer/ContentDirectory/Control", PSoapValues(new SoapValues()))));
	_upnpFunctions.insert(UpnpFunctionPair("GetCrossfadeMode", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0") }))));
	_upnpFunctions.insert(UpnpFunctionPair("GetMute", UpnpFunctionEntry("urn:schemas-upnp-org:service:RenderingControl:1", "/MediaRenderer/RenderingControl/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master") }))));
	_upnpFunctions.insert(UpnpFunctionPair("GetMediaInfo", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0") }))));
	_upnpFunctions.insert(UpnpFunctionPair("GetPositionInfo", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0") }))));
	_upnpFunctions.insert(UpnpFunctionPair("GetRemainingSleepTimerDuration", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0") }))));
	_upnpFunctions.insert(UpnpFunctionPair("GetTransportInfo", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0") }))));
	_upnpFunctions.insert(UpnpFunctionPair("GetVolume", UpnpFunctionEntry("urn:schemas-upnp-org:service:RenderingControl:1", "/MediaRenderer/RenderingControl/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master") }))));
	_upnpFunctions.insert(UpnpFunctionPair("Next", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0") }))));
	_upnpFunctions.insert(UpnpFunctionPair("Pause", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0") }))));
	_upnpFunctions.insert(UpnpFunctionPair("Play", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Speed", "1") }))));
	_upnpFunctions.insert(UpnpFunctionPair("Previous", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0") }))));
	_upnpFunctions.insert(UpnpFunctionPair("RampToVolume", UpnpFunctionEntry("urn:schemas-upnp-org:service:RenderingControl:1", "/MediaRenderer/RenderingControl/Control", PSoapValues(new SoapValues()))));
	_upnpFunctions.insert(UpnpFunctionPair("RemoveAllTracksFromQueue", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0") }))));
	_upnpFunctions.insert(UpnpFunctionPair("RemoveTrackFromQueue", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues()))));
	_upnpFunctions.insert(UpnpFunctionPair("Seek", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues()))));
	_upnpFunctions.insert(UpnpFunctionPair("SetAVTransportURI", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues()))));
	_upnpFunctions.insert(UpnpFunctionPair("SetCrossfadeMode", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues()))));
	_upnpFunctions.insert(UpnpFunctionPair("SetMute", UpnpFunctionEntry("urn:schemas-upnp-org:service:RenderingControl:1", "/MediaRenderer/RenderingControl/Control", PSoapValues(new SoapValues()))));
	_upnpFunctions.insert(UpnpFunctionPair("SetPlayMode", UpnpFunctionEntry("urn:schemas-upnp-org:service:AVTransport:1", "/MediaRenderer/AVTransport/Control", PSoapValues(new SoapValues()))));
	_upnpFunctions.insert(UpnpFunctionPair("SetVolume", UpnpFunctionEntry("urn:schemas-upnp-org:service:RenderingControl:1", "/MediaRenderer/RenderingControl/Control", PSoapValues(new SoapValues()))));
}

void SonosPeer::setIp(std::string value)
{
	try
	{
		Peer::setIp(value);
		_httpClient.reset(new BaseLib::HttpClient(GD::bl, _ip, 1400, false));
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

std::string SonosPeer::getRinconId()
{
	try
	{
		if(!_rpcDevice) return "";
		Functions::iterator functionIterator = _rpcDevice->functions.find(1);
		if(functionIterator == _rpcDevice->functions.end()) return "";
		PParameter parameter = functionIterator->second->variables->getParameter("ID");
		if(!parameter) return "";
		return parameter->convertFromPacket(valuesCentral[1]["ID"].data)->stringValue;
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
	return "";
}

void SonosPeer::setRinconId(std::string value)
{
	try
	{
		if(!_rpcDevice) return;
		Functions::iterator functionIterator = _rpcDevice->functions.find(1);
		if(functionIterator == _rpcDevice->functions.end()) return;
		PParameter parameter = functionIterator->second->variables->getParameter("ID");
		if(!parameter) return;
		BaseLib::Systems::RPCConfigurationParameter& parameterData = valuesCentral[1]["ID"];
		parameter->convertToPacket(PVariable(new Variable(value)), parameterData.data);
		if(parameterData.databaseID > 0) saveParameter(parameterData.databaseID, parameterData.data);
		else saveParameter(0, ParameterGroup::Type::Enum::variables, 1, "ID", parameterData.data);
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

void SonosPeer::setRoomName(std::string value, bool broadCastEvent)
{
	try
	{
		if(!_rpcDevice) return;
		Functions::iterator functionIterator = _rpcDevice->functions.find(1);
		if(functionIterator == _rpcDevice->functions.end()) return;
		PParameter parameter = functionIterator->second->variables->getParameter("ROOMNAME");
		if(!parameter) return;
		PVariable variable(new Variable(value));
		BaseLib::Systems::RPCConfigurationParameter& parameterData = valuesCentral[1]["ROOMNAME"];
		parameter->convertToPacket(variable, valuesCentral[1]["ROOMNAME"].data);
		if(parameterData.databaseID > 0) saveParameter(parameterData.databaseID, parameterData.data);
		else saveParameter(0, ParameterGroup::Type::Enum::variables, 1, "ROOMNAME", parameterData.data);

		if(broadCastEvent)
		{
			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{ "ROOMNAME" });
			std::shared_ptr<std::vector<PVariable>> values(new std::vector<PVariable>{ variable });
			raiseEvent(_peerID, 1, valueKeys, values);
			raiseRPCEvent(_peerID, 1, _serialNumber + ":" + std::to_string(1), valueKeys, values);
		}
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

void SonosPeer::worker()
{
	try
	{
		if(_shuttingDown || deleting) return;
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>());
		std::shared_ptr<std::vector<PVariable>> rpcValues(new std::vector<PVariable>());
		if( BaseLib::HelperFunctions::getTimeSeconds() - _lastPositionInfo > 5)
		{
			_lastPositionInfo = BaseLib::HelperFunctions::getTimeSeconds();
			//Get position info if TRANSPORT_STATE is PLAYING
			std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelIterator = valuesCentral.find(1);
			if(channelIterator != valuesCentral.end())
			{
				std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelIterator->second.find("TRANSPORT_STATE");
				if(parameterIterator != channelIterator->second.end())
				{
					PVariable transportState = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(transportState)
					{
						if(transportState->stringValue == "PLAYING" || _getOneMorePositionInfo)
						{
							_getOneMorePositionInfo = (transportState->stringValue == "PLAYING");
							execute("GetPositionInfo");
						}
					}
				}
			}
		}
		if( BaseLib::HelperFunctions::getTimeSeconds() - _lastAvTransportInfo > 60)
		{
			_lastAvTransportInfo = BaseLib::HelperFunctions::getTimeSeconds();
			execute("GetMediaInfo");
		}
		if(BaseLib::HelperFunctions::getTimeSeconds() - _lastAvTransportSubscription > 300)
		{
			_lastAvTransportSubscription = BaseLib::HelperFunctions::getTimeSeconds();
			std::vector<std::string> subscriptionPackets(7);
			subscriptionPackets.at(0) = "SUBSCRIBE /ZoneGroupTopology/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			subscriptionPackets.at(1) = "SUBSCRIBE /MediaRenderer/RenderingControl/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			subscriptionPackets.at(2) = "SUBSCRIBE /MediaRenderer/AVTransport/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			subscriptionPackets.at(3) = "SUBSCRIBE /MediaServer/ContentDirectory/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			subscriptionPackets.at(4) = "SUBSCRIBE /AlarmClock/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			subscriptionPackets.at(5) = "SUBSCRIBE /SystemProperties/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			subscriptionPackets.at(6) = "SUBSCRIBE /MusicServices/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			if(_httpClient)
			{
				for(int32_t i = 0; i < subscriptionPackets.size(); i++)
				{
					std::string response;
					try
					{
						_httpClient->sendRequest(subscriptionPackets.at(i), response, true);
						if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + response);
						serviceMessages->setUnreach(false, true);
					}
					catch(BaseLib::HttpClientException& ex)
					{
						GD::out.printInfo("Info: Error calling SUBSCRIBE (" + std::to_string(i) + ") on Sonos device: " + ex.what());
						if(ex.responseCode() == -1) serviceMessages->setUnreach(true, false);
					}
					catch(BaseLib::Exception& ex)
					{
						GD::out.printInfo("Info: Error calling SUBSCRIBE (" + std::to_string(i) + ") on Sonos device: " + ex.what());
					}
					catch(const std::exception& ex)
					{
						GD::out.printInfo("Info: Error calling SUBSCRIBE (" + std::to_string(i) + ") on Sonos device: " + ex.what());
					}
				}
			}
		}
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

void SonosPeer::homegearShuttingDown()
{
	try
	{
		_shuttingDown = true;
		Peer::homegearShuttingDown();
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

std::string SonosPeer::handleCliCommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;

		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "unselect\t\tUnselect this peer" << std::endl;
			stringStream << "channel count\t\tPrint the number of channels of this peer" << std::endl;
			stringStream << "config print\t\tPrints all configuration parameters and their values" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 13, "channel count") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints this peer's number of channels." << std::endl;
						stringStream << "Usage: channel count" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			stringStream << "Peer has " << _rpcDevice->functions.size() << " channels." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 12, "config print") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints all configuration parameters of this peer. The values are in BidCoS packet format." << std::endl;
						stringStream << "Usage: config print" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			return printConfig();
		}
		else return "Unknown command.\n";
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

void SonosPeer::addPeer(std::shared_ptr<BaseLib::Systems::BasicPeer> peer)
{
	try
	{

		if(_rpcDevice->functions.find(1) == _rpcDevice->functions.end()) return;
		std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>& channel1Peers = _peers[1];
		for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = channel1Peers.begin(); i != channel1Peers.end(); ++i)
		{
			if((*i)->id == peer->id)
			{
				channel1Peers.erase(i);
				break;
			}
		}
		channel1Peers.push_back(peer);
		savePeers();
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

void SonosPeer::removePeer(uint64_t id)
{
	try
	{
		if(_peers.find(1) == _peers.end()) return;
		std::shared_ptr<SonosCentral> central(std::dynamic_pointer_cast<SonosCentral>(getCentral()));

		std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>& channel1Peers = _peers[1];
		for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = channel1Peers.begin(); i != channel1Peers.end(); ++i)
		{
			if((*i)->id == id)
			{
				channel1Peers.erase(i);
				savePeers();
				return;
			}
		}
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

void SonosPeer::savePeers()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializePeers(serializedData);
		saveVariable(12, serializedData);
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

void SonosPeer::serializePeers(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder(_bl);
		encoder.encodeInteger(encodedData, _peers.size());
		for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			encoder.encodeInteger(encodedData, i->first);
			encoder.encodeInteger(encodedData, i->second.size());
			for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(!*j) continue;
				encoder.encodeBoolean(encodedData, (*j)->isSender);
				encoder.encodeInteger(encodedData, (*j)->id);
				encoder.encodeInteger(encodedData, (*j)->address);
				encoder.encodeInteger(encodedData, (*j)->channel);
				encoder.encodeString(encodedData, (*j)->serialNumber);
				encoder.encodeBoolean(encodedData, (*j)->isVirtual);
				encoder.encodeString(encodedData, (*j)->linkName);
				encoder.encodeString(encodedData, (*j)->linkDescription);
				encoder.encodeInteger(encodedData, (*j)->data.size());
				encodedData.insert(encodedData.end(), (*j)->data.begin(), (*j)->data.end());
			}
		}
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

void SonosPeer::unserializePeers(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BaseLib::BinaryDecoder decoder(_bl);
		uint32_t position = 0;
		uint32_t peersSize = decoder.decodeInteger(*serializedData, position);
		for(uint32_t i = 0; i < peersSize; i++)
		{
			uint32_t channel = decoder.decodeInteger(*serializedData, position);
			uint32_t peerCount = decoder.decodeInteger(*serializedData, position);
			for(uint32_t j = 0; j < peerCount; j++)
			{
				std::shared_ptr<BaseLib::Systems::BasicPeer> basicPeer(new BaseLib::Systems::BasicPeer());
				basicPeer->hasSender = true;
				basicPeer->isSender = decoder.decodeBoolean(*serializedData, position);
				basicPeer->id = decoder.decodeInteger(*serializedData, position);
				basicPeer->address = decoder.decodeInteger(*serializedData, position);
				basicPeer->channel = decoder.decodeInteger(*serializedData, position);
				basicPeer->serialNumber = decoder.decodeString(*serializedData, position);
				basicPeer->isVirtual = decoder.decodeBoolean(*serializedData, position);
				_peers[channel].push_back(basicPeer);
				basicPeer->linkName = decoder.decodeString(*serializedData, position);
				basicPeer->linkDescription = decoder.decodeString(*serializedData, position);
				uint32_t dataSize = decoder.decodeInteger(*serializedData, position);
				if(position + dataSize <= serializedData->size()) basicPeer->data.insert(basicPeer->data.end(), serializedData->begin() + position, serializedData->begin() + position + dataSize);
				position += dataSize;
			}
		}
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

std::string SonosPeer::printConfig()
{
	try
	{
		std::ostringstream stringStream;
		stringStream << "MASTER" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::const_iterator i = configCentral.begin(); i != configCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t[" << j->first << "]: ";
				if(!j->second.rpcParameter) stringStream << "(No RPC parameter) ";
				for(std::vector<uint8_t>::const_iterator k = j->second.data.begin(); k != j->second.data.end(); ++k)
				{
					stringStream << std::hex << std::setfill('0') << std::setw(2) << (int32_t)*k << " ";
				}
				stringStream << std::endl;
			}
			stringStream << "\t}" << std::endl;
		}
		stringStream << "}" << std::endl << std::endl;

		stringStream << "VALUES" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::const_iterator i = valuesCentral.begin(); i != valuesCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t[" << j->first << "]: ";
				if(!j->second.rpcParameter) stringStream << "(No RPC parameter) ";
				for(std::vector<uint8_t>::const_iterator k = j->second.data.begin(); k != j->second.data.end(); ++k)
				{
					stringStream << std::hex << std::setfill('0') << std::setw(2) << (int32_t)*k << " ";
				}
				stringStream << std::endl;
			}
			stringStream << "\t}" << std::endl;
		}
		stringStream << "}" << std::endl << std::endl;

		return stringStream.str();
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
    return "";
}


void SonosPeer::loadVariables(BaseLib::Systems::ICentral* central, std::shared_ptr<BaseLib::Database::DataTable>& rows)
{
	try
	{
		if(!rows) rows = _bl->db->getPeerVariables(_peerID);
		Peer::loadVariables(central, rows);
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 1:
				_ip = row->second.at(4)->textValue;
				_httpClient.reset(new BaseLib::HttpClient(GD::bl, _ip, 1400, false));
				break;
			case 12:
				unserializePeers(row->second.at(5)->binaryValue);
				break;
			}
		}
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

bool SonosPeer::load(BaseLib::Systems::ICentral* central)
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows;
		loadVariables(central, rows);

		_rpcDevice = GD::family->getRpcDevices()->find(_deviceType, 0x10, -1);
		if(!_rpcDevice)
		{
			GD::out.printError("Error loading Sonos peer " + std::to_string(_peerID) + ": Device type not found: 0x" + BaseLib::HelperFunctions::getHexString((uint32_t)_deviceType.type()) + " Firmware version: " + std::to_string(_firmwareVersion));
			return false;
		}
		initializeTypeString();
		std::string entry;
		loadConfig();
		initializeCentralConfig();

		serviceMessages.reset(new BaseLib::Systems::ServiceMessages(_bl, _peerID, _serialNumber, this));
		serviceMessages->load();

		std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
		if(channelOneIterator != valuesCentral.end())
		{
			std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("VOLUME");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) _currentVolume = variable->integerValue;
			}
		}

		return true;
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
    return false;
}

void SonosPeer::saveVariables()
{
	try
	{
		if(_peerID == 0) return;
		Peer::saveVariables();
		saveVariable(1, _ip);
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

void SonosPeer::getValuesFromPacket(std::shared_ptr<SonosPacket> packet, std::vector<FrameValues>& frameValues)
{
	try
	{
		if(!_rpcDevice) return;
		//equal_range returns all elements with "0" or an unknown element as argument
		if(_rpcDevice->packetsByFunction2.find(packet->functionName()) == _rpcDevice->packetsByFunction2.end()) return;
		std::pair<PacketsByFunction::iterator, PacketsByFunction::iterator> range = _rpcDevice->packetsByFunction2.equal_range(packet->functionName());
		if(range.first == _rpcDevice->packetsByFunction2.end()) return;
		PacketsByFunction::iterator i = range.first;
		std::shared_ptr<std::unordered_map<std::string, std::string>> soapValues;
		std::string field;
		do
		{
			FrameValues currentFrameValues;
			PPacket frame(i->second);
			if(!frame) continue;
			if(frame->direction != Packet::Direction::Enum::toCentral) continue;
			int32_t channel = -1;
			if(frame->channel > -1) channel = frame->channel;
			currentFrameValues.frameID = frame->id;

			for(JsonPayloads::iterator j = frame->jsonPayloads.begin(); j != frame->jsonPayloads.end(); ++j)
			{
				if(!(*j)->subkey.empty() && ((*j)->key == "CurrentTrackMetaData" || (*j)->key == "TrackMetaData"))
				{
					soapValues = packet->currentTrackMetadata();
					if(!soapValues || soapValues->find((*j)->subkey) == soapValues->end()) continue;
					field = (*j)->subkey;
				}
				else if(!(*j)->subkey.empty() && ((*j)->key == "r:NextTrackMetaData"))
				{
					soapValues = packet->nextTrackMetadata();
					if(!soapValues || soapValues->find((*j)->subkey) == soapValues->end()) continue;
					field = (*j)->subkey;
				}
				else if(!(*j)->subkey.empty() && ((*j)->key == "AVTransportURIMetaData"))
				{
					soapValues = packet->avTransportUriMetaData();
					if(!soapValues || soapValues->find((*j)->subkey) == soapValues->end()) continue;
					field = (*j)->subkey;
				}
				else if(!(*j)->subkey.empty() && ((*j)->key == "NextAVTransportURIMetaData"))
				{
					soapValues = packet->nextAvTransportUriMetaData();
					if(!soapValues || soapValues->find((*j)->subkey) == soapValues->end()) continue;
					field = (*j)->subkey;
				}
				else if(!(*j)->subkey.empty() && ((*j)->key == "r:EnqueuedTransportURIMetaData"))
				{
					soapValues = packet->currentTrackMetadata();
					if(!soapValues || soapValues->find((*j)->subkey) == soapValues->end()) continue;
					field = (*j)->subkey;
				}
				else
				{
					soapValues = packet->values();
					if((!soapValues || soapValues->find((*j)->key) == soapValues->end()) && (!packet->browseResult() || packet->browseResult()->first != (*j)->key)) continue;
					field = (*j)->key;
				}

				for(std::vector<PParameter>::iterator k = frame->associatedVariables.begin(); k != frame->associatedVariables.end(); ++k)
				{
					if((*k)->physical->groupId != (*j)->parameterId) continue;
					currentFrameValues.parameterSetType = (*k)->parent()->type();
					bool setValues = false;
					if(currentFrameValues.paramsetChannels.empty()) //Fill paramsetChannels
					{
						int32_t startChannel = (channel < 0) ? 0 : channel;
						int32_t endChannel;
						//When fixedChannel is -2 (means '*') cycle through all channels
						if(frame->channel == -2)
						{
							startChannel = 0;
							endChannel = _rpcDevice->functions.rbegin()->first;
						}
						else endChannel = startChannel;
						for(int32_t l = startChannel; l <= endChannel; l++)
						{
							Functions::iterator functionIterator = _rpcDevice->functions.find(l);
							if(functionIterator == _rpcDevice->functions.end()) continue;
							PParameterGroup parameterGroup = functionIterator->second->getParameterGroup(currentFrameValues.parameterSetType);
							Parameters::iterator parameterIterator = parameterGroup->parameters.find((*k)->id);
							if(parameterIterator == parameterGroup->parameters.end()) continue;
							currentFrameValues.paramsetChannels.push_back(l);
							currentFrameValues.values[(*k)->id].channels.push_back(l);
							setValues = true;
						}
					}
					else //Use paramsetChannels
					{
						for(std::list<uint32_t>::const_iterator l = currentFrameValues.paramsetChannels.begin(); l != currentFrameValues.paramsetChannels.end(); ++l)
						{
							Functions::iterator functionIterator = _rpcDevice->functions.find(*l);
							if(functionIterator == _rpcDevice->functions.end()) continue;
							PParameterGroup parameterGroup = functionIterator->second->getParameterGroup(currentFrameValues.parameterSetType);
							Parameters::iterator parameterIterator = parameterGroup->parameters.find((*k)->id);
							if(parameterIterator == parameterGroup->parameters.end()) continue;
							currentFrameValues.values[(*k)->id].channels.push_back(*l);
							setValues = true;
						}
					}

					if(setValues)
					{
						//This is a little nasty and costs a lot of resources, but we need to run the data through the packet converter
						std::vector<uint8_t> encodedData;
						if(packet->browseResult()) _binaryEncoder->encodeResponse(packet->browseResult()->second, encodedData);
						else _binaryEncoder->encodeResponse(Variable::fromString(soapValues->at(field), (*k)->physical->type), encodedData);
						PVariable data = (*k)->convertFromPacket(encodedData, true);
						(*k)->convertToPacket(data, currentFrameValues.values[(*k)->id].value);
					}
				}
			}
			if(!currentFrameValues.values.empty()) frameValues.push_back(currentFrameValues);
		} while(++i != range.second && i != _rpcDevice->packetsByFunction2.end());
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

void SonosPeer::packetReceived(std::shared_ptr<SonosPacket> packet)
{
	try
	{
		if(!packet) return;
		if(_disposing) return;
		if(!_rpcDevice) return;
		setLastPacketReceived();
		std::vector<FrameValues> frameValues;
		getValuesFromPacket(packet, frameValues);
		std::map<uint32_t, std::shared_ptr<std::vector<std::string>>> valueKeys;
		std::map<uint32_t, std::shared_ptr<std::vector<PVariable>>> rpcValues;

		//Loop through all matching frames
		for(std::vector<FrameValues>::iterator a = frameValues.begin(); a != frameValues.end(); ++a)
		{
			PPacket frame;
			if(!a->frameID.empty()) frame = _rpcDevice->packetsById.at(a->frameID);

			for(std::unordered_map<std::string, FrameValue>::iterator i = a->values.begin(); i != a->values.end(); ++i)
			{
				for(std::list<uint32_t>::const_iterator j = a->paramsetChannels.begin(); j != a->paramsetChannels.end(); ++j)
				{
					if(std::find(i->second.channels.begin(), i->second.channels.end(), *j) == i->second.channels.end()) continue;

					BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[*j][i->first];
					if(parameter->data == i->second.value) continue;

					if(!valueKeys[*j] || !rpcValues[*j])
					{
						valueKeys[*j].reset(new std::vector<std::string>());
						rpcValues[*j].reset(new std::vector<PVariable>());
					}

					if(!valueKeys[1] || !rpcValues[1])
					{
						valueKeys[1].reset(new std::vector<std::string>());
						rpcValues[1].reset(new std::vector<PVariable>());
					}

					if(parameter->rpcParameter)
					{
						//Process service messages
						if(parameter->rpcParameter->service && !i->second.value.empty())
						{
							if(parameter->rpcParameter->logical->type == ILogical::Type::Enum::tEnum)
							{
								serviceMessages->set(i->first, i->second.value.at(i->second.value.size() - 1), *j);
							}
							else if(parameter->rpcParameter->logical->type == ILogical::Type::Enum::tBoolean)
							{
								serviceMessages->set(i->first, (bool)i->second.value.at(i->second.value.size() - 1));
							}
						}

						PVariable value = parameter->rpcParameter->convertFromPacket(i->second.value, true);
						if(i->first == "CURRENT_TRACK") _currentTrack = value->integerValue;
						if(i->first == "CURRENT_TRACK_URI" && value->stringValue.empty())
						{
							//Clear current track information when uri is empty

							BaseLib::Systems::RPCConfigurationParameter* parameter2 = &valuesCentral[1]["CURRENT_TITLE"];
							parameter2->data.clear();
							if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
							valueKeys[1]->push_back("CURRENT_TITLE");
							rpcValues[1]->push_back(value);

							parameter2 = &valuesCentral[1]["CURRENT_ALBUM"];
							parameter2->data.clear();
							if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
							valueKeys[1]->push_back("CURRENT_ALBUM");
							rpcValues[1]->push_back(value);

							parameter2 = &valuesCentral[1]["CURRENT_ARTIST"];
							parameter2->data.clear();
							if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
							valueKeys[1]->push_back("CURRENT_ARTIST");
							rpcValues[1]->push_back(value);

							parameter2 = &valuesCentral[1]["CURRENT_ALBUM_ART"];
							parameter2->data.clear();
							if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
							valueKeys[1]->push_back("CURRENT_ALBUM_ART");
							rpcValues[1]->push_back(value);
						}
						else if(i->first == "NEXT_TRACK_URI" && value->stringValue.empty())
						{
							//Clear next track information when uri is empty

							BaseLib::Systems::RPCConfigurationParameter* parameter2 = &valuesCentral[1]["NEXT_TITLE"];
							parameter2->data.clear();
							if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
							valueKeys[1]->push_back("NEXT_TITLE");
							rpcValues[1]->push_back(value);

							parameter2 = &valuesCentral[1]["NEXT_ALBUM"];
							parameter2->data.clear();
							if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
							valueKeys[1]->push_back("NEXT_ALBUM");
							rpcValues[1]->push_back(value);

							parameter2 = &valuesCentral[1]["NEXT_ARTIST"];
							parameter2->data.clear();
							if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
							valueKeys[1]->push_back("NEXT_ARTIST");
							rpcValues[1]->push_back(value);

							parameter2 = &valuesCentral[1]["NEXT_ALBUM_ART"];
							parameter2->data.clear();
							if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
							valueKeys[1]->push_back("NEXT_ALBUM_ART");
							rpcValues[1]->push_back(value);
						}
						else if(i->first == "AV_TRANSPORT_URI" && value->stringValue.size() > 0)
						{
							std::shared_ptr<SonosCentral> central = std::dynamic_pointer_cast<SonosCentral>(getCentral());
							BaseLib::PVariable oldValue = parameter->rpcParameter->convertFromPacket(parameter->data, true);

							//Update IS_MASTER
							BaseLib::Systems::RPCConfigurationParameter* parameter2 = &valuesCentral[1]["IS_MASTER"];
							if(parameter2->rpcParameter)
							{
								BaseLib::PVariable isMaster(new BaseLib::Variable(value->stringValue.size() < 9 || value->stringValue.compare(0, 9, "x-rincon:") != 0));
								if(parameter2->data.empty() || (bool)parameter2->data.back() != isMaster->booleanValue)
								{
									parameter2->rpcParameter->convertToPacket(isMaster, parameter2->data);
									if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
									else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
									valueKeys[1]->push_back("IS_MASTER");
									rpcValues[1]->push_back(isMaster);
								}
							}

							//Update links
							if(oldValue->stringValue.size() > 9 && oldValue->stringValue.compare(0, 9, "x-rincon:") == 0 && oldValue->stringValue != value->stringValue)
							{
								std::string oldRinconId = oldValue->stringValue.substr(9);
								std::shared_ptr<SonosPeer> oldPeer = central->getPeerByRinconId(oldRinconId);
								if(oldPeer)
								{
									oldPeer->removePeer(_peerID);
									removePeer(oldPeer->getID());
								}
								if(value->stringValue.size() > 9 && value->stringValue.compare(0, 9, "x-rincon:") == 0)
								{
									std::string newRinconId = value->stringValue.substr(9);
									std::shared_ptr<SonosPeer> newPeer = central->getPeerByRinconId(newRinconId);
									if(newPeer)
									{
										std::shared_ptr<BaseLib::Systems::BasicPeer> senderPeer(new BaseLib::Systems::BasicPeer());
										senderPeer->address = newPeer->getAddress();
										senderPeer->channel = 1;
										senderPeer->id = newPeer->getID();
										senderPeer->serialNumber = newPeer->getSerialNumber();
										senderPeer->hasSender = true;
										senderPeer->isSender = true;
										addPeer(senderPeer);

										std::shared_ptr<BaseLib::Systems::BasicPeer> receiverPeer(new BaseLib::Systems::BasicPeer());
										receiverPeer->address = _address;
										receiverPeer->channel = 1;
										receiverPeer->id = _peerID;
										receiverPeer->serialNumber = _serialNumber;
										receiverPeer->hasSender = true;
										newPeer->addPeer(receiverPeer);
									}
								}
							}
							else if(value->stringValue.size() > 9 && value->stringValue.compare(0, 9, "x-rincon:") == 0)
							{
								std::string newRinconId = value->stringValue.substr(9);
								std::shared_ptr<SonosPeer> newPeer = central->getPeerByRinconId(newRinconId);
								if(newPeer)
								{
									std::shared_ptr<BaseLib::Systems::BasicPeer> senderPeer(new BaseLib::Systems::BasicPeer());
									senderPeer->address = newPeer->getAddress();
									senderPeer->channel = 1;
									senderPeer->id = newPeer->getID();
									senderPeer->serialNumber = newPeer->getSerialNumber();
									senderPeer->hasSender = true;
									senderPeer->isSender = true;
									addPeer(senderPeer);

									std::shared_ptr<BaseLib::Systems::BasicPeer> receiverPeer(new BaseLib::Systems::BasicPeer());
									receiverPeer->address = _address;
									receiverPeer->channel = 1;
									receiverPeer->id = _peerID;
									receiverPeer->serialNumber = _serialNumber;
									receiverPeer->hasSender = true;
									newPeer->addPeer(receiverPeer);
								}
							}

							if(value->stringValue.compare(0, 18, "x-sonosapi-stream:") == 0)
							{
								//Delete track information for radio

								PVariable value(new Variable(VariableType::tString));
								BaseLib::Systems::RPCConfigurationParameter* parameter2 = &valuesCentral[1]["CURRENT_ALBUM"];
								parameter2->data.clear();
								if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
								else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
								valueKeys[1]->push_back("CURRENT_ALBUM");
								rpcValues[1]->push_back(value);

								parameter2 = &valuesCentral[1]["CURRENT_ARTIST"];
								parameter2->data.clear();
								if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
								else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
								valueKeys[1]->push_back("CURRENT_ARTIST");
								rpcValues[1]->push_back(value);

								parameter2 = &valuesCentral[1]["CURRENT_ALBUM_ART"];
								parameter2->data.clear();
								if(parameter2->databaseID > 0) saveParameter(parameter2->databaseID, parameter2->data);
								else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter2->data);
								valueKeys[1]->push_back("CURRENT_ALBUM_ART");
								rpcValues[1]->push_back(value);
							}
						}
						else if(i->first == "VOLUME") _currentVolume = value->integerValue;

						parameter->data = i->second.value;
						if(parameter->databaseID > 0) saveParameter(parameter->databaseID, parameter->data);
						else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, parameter->data);
						if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + i->first + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(*j) + " was set.");

						valueKeys[*j]->push_back(i->first);
						rpcValues[*j]->push_back(value);
					}
				}
			}
		}

		if(!rpcValues.empty())
		{
			for(std::map<uint32_t, std::shared_ptr<std::vector<std::string>>>::const_iterator j = valueKeys.begin(); j != valueKeys.end(); ++j)
			{
				if(j->second->empty()) continue;

				std::string address(_serialNumber + ":" + std::to_string(j->first));
				raiseEvent(_peerID, j->first, j->second, rpcValues.at(j->first));
				raiseRPCEvent(_peerID, j->first, address, j->second, rpcValues.at(j->first));
			}
		}
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

void SonosPeer::sendSoapRequest(std::string& request, bool ignoreErrors)
{
	try
	{
		if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: Sending SOAP request:\n" + request);
		if(_httpClient)
		{
			std::string response;
			try
			{
				_httpClient->sendRequest(request, response);
				if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + response);
				std::shared_ptr<SonosPacket> responsePacket(new SonosPacket(response));
				packetReceived(responsePacket);
				serviceMessages->setUnreach(false, true);
			}
			catch(BaseLib::HttpClientException& ex)
			{
				if(ignoreErrors) return;
				GD::out.printWarning("Warning: Error in UPnP request: " + ex.what());
				GD::out.printMessage("Request was: \n" + request);
				if(ex.responseCode() == -1) serviceMessages->setUnreach(true, false);
			}
			catch(BaseLib::Exception& ex)
			{
				if(ignoreErrors) return;
				GD::out.printWarning("Warning: Error in UPnP request: " + ex.what());
				GD::out.printMessage("Request was: \n" + request);
				serviceMessages->setUnreach(true, false);
			}
		}
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

void SonosPeer::execute(std::string& functionName, std::string& service, std::string& path, std::shared_ptr<std::vector<std::pair<std::string, std::string>>>& soapValues, bool ignoreErrors)
{
	try
	{
		std::string soapRequest;
		std::string headerSoapRequest = service + '#' + functionName;
		SonosPacket packet(_ip, path, headerSoapRequest, service, functionName, soapValues);
		packet.getSoapRequest(soapRequest);
		sendSoapRequest(soapRequest);
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

void SonosPeer::execute(std::string functionName, bool ignoreErrors)
{
	try
	{
		UpnpFunctions::iterator functionEntry = _upnpFunctions.find(functionName);
		if(functionEntry == _upnpFunctions.end())
		{
			GD::out.printError("Error: Tried to execute unknown function: " + functionName);
			return;
		}
		std::string soapRequest;
		std::string headerSoapRequest = functionEntry->second.service() + '#' + functionName;
		SonosPacket packet(_ip, functionEntry->second.path(), headerSoapRequest, functionEntry->second.service(), functionName, functionEntry->second.soapValues());
		packet.getSoapRequest(soapRequest);
		sendSoapRequest(soapRequest, ignoreErrors);
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

void SonosPeer::execute(std::string functionName, PSoapValues soapValues, bool ignoreErrors)
{
	try
	{
		UpnpFunctions::iterator functionEntry = _upnpFunctions.find(functionName);
		if(functionEntry == _upnpFunctions.end())
		{
			GD::out.printError("Error: Tried to execute unknown function: " + functionName);
			return;
		}
		std::string soapRequest;
		std::string headerSoapRequest = functionEntry->second.service() + '#' + functionName;
		SonosPacket packet(_ip, functionEntry->second.path(), headerSoapRequest, functionEntry->second.service(), functionName, soapValues);
		packet.getSoapRequest(soapRequest);
		sendSoapRequest(soapRequest, ignoreErrors);
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

PVariable SonosPeer::playBrowsableContent(std::string& title, std::string browseId, std::string listVariable)
{
	try
	{
		std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
		if(channelOneIterator == valuesCentral.end())
		{
			GD::out.printError("Error: Channel 1 not found.");
			return Variable::createError(-32500, "Channel 1 not found.");
		}

		execute("Browse", PSoapValues(new SoapValues{ SoapValuePair("ObjectID", browseId), SoapValuePair("BrowseFlag", "BrowseDirectChildren"), SoapValuePair("Filter", ""), SoapValuePair("StartingIndex", "0"), SoapValuePair("RequestedCount", "0"), SoapValuePair("SortCriteria", "") }));

		PVariable entries;
		std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find(listVariable);
		if(parameterIterator != channelOneIterator->second.end())
		{
			entries = _binaryDecoder->decodeResponse(parameterIterator->second.data);
		}
		if(!entries) return Variable::createError(-32500, "Data could not be decoded.");

		std::string uri;
		std::string metadata;
		for(Array::iterator i = entries->arrayValue->begin(); i != entries->arrayValue->end(); ++i)
		{
			if((*i)->type != VariableType::tStruct) continue;
			Struct::iterator structIterator = (*i)->structValue->find("TITLE");
			if(structIterator == (*i)->structValue->end() || structIterator->second->stringValue != title) continue;
			structIterator = (*i)->structValue->find("AV_TRANSPORT_URI");
			if(structIterator == (*i)->structValue->end()) continue;
			uri = structIterator->second->stringValue;
			structIterator = (*i)->structValue->find("AV_TRANSPORT_URI_METADATA");
			if(structIterator == (*i)->structValue->end()) continue;
			metadata = structIterator->second->stringValue;
		}
		if(uri.empty() && metadata.empty()) return Variable::createError(-2, "No entry with this name found.");

		std::string rinconId;
		parameterIterator = channelOneIterator->second.find("ID");
		if(parameterIterator != channelOneIterator->second.end())
		{
			PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
			if(variable) rinconId = variable->stringValue;
		}
		if(rinconId.empty()) return Variable::createError(-32500, "Rincon id could not be decoded.");

		execute("Pause", true);
		execute("RemoveAllTracksFromQueue", true);
		execute("SetMute", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("DesiredMute", "0") }));
		execute("AddURIToQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("EnqueuedURI", uri), SoapValuePair("EnqueuedURIMetaData", metadata), SoapValuePair("DesiredFirstTrackNumberEnqueued", "0"), SoapValuePair("EnqueueAsNext", "0") }));
		execute("SetAVTransportURI", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("CurrentURI", "x-rincon-queue:" + rinconId + "#0"), SoapValuePair("CurrentURIMetaData", "") }));
		execute("Seek", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Unit", "TRACK_NR"), SoapValuePair("Target", std::to_string(1)) }));
		execute("Play", true);
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

PVariable SonosPeer::getValueFromDevice(PParameter& parameter, int32_t channel, bool asynchronous)
{
	try
	{
		if(!parameter) return Variable::createError(-32500, "parameter is nullptr.");
		if(parameter->getPackets.empty()) return Variable::createError(-6, "Parameter can't be requested actively.");
		std::string getRequestFrame = parameter->getPackets.front()->id;
		std::string getResponseFrame = parameter->getPackets.front()->responseId;
		if(_rpcDevice->packetsById.find(getRequestFrame) == _rpcDevice->packetsById.end()) return Variable::createError(-6, "No frame was found for parameter " + parameter->id);
		PPacket frame = _rpcDevice->packetsById[getRequestFrame];
		PPacket responseFrame;
		if(_rpcDevice->packetsById.find(getResponseFrame) != _rpcDevice->packetsById.end()) responseFrame = _rpcDevice->packetsById[getResponseFrame];

		if(valuesCentral.find(channel) == valuesCentral.end()) return Variable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(parameter->id) == valuesCentral[channel].end()) return Variable::createError(-5, "Unknown parameter.");

		PParameterGroup parameterGroup = getParameterSet(channel, ParameterGroup::Type::Enum::variables);
		if(!parameterGroup) return Variable::createError(-3, "Unknown parameter set.");

		std::shared_ptr<std::vector<std::pair<std::string, std::string>>> soapValues(new std::vector<std::pair<std::string, std::string>>());
		for(JsonPayloads::iterator i = frame->jsonPayloads.begin(); i != frame->jsonPayloads.end(); ++i)
		{
			if((*i)->constValueInteger > -1)
			{
				if((*i)->key.empty()) continue;
				soapValues->push_back(std::pair<std::string, std::string>((*i)->key, std::to_string((*i)->constValueInteger)));
				continue;
			}
			else if((*i)->constValueStringSet)
			{
				if((*i)->key.empty()) continue;
				soapValues->push_back(std::pair<std::string, std::string>((*i)->key, (*i)->constValueString));
				continue;
			}

			bool paramFound = false;
			for(Parameters::iterator j = parameterGroup->parameters.begin(); j != parameterGroup->parameters.end(); ++j)
			{
				if((*i)->parameterId == j->second->physical->groupId)
				{
					if((*i)->key.empty()) continue;
					soapValues->push_back(std::pair<std::string, std::string>((*i)->key, _binaryDecoder->decodeResponse((valuesCentral[channel][j->second->id].data))->toString()));
					paramFound = true;
					break;
				}
			}
			if(!paramFound) GD::out.printError("Error constructing packet. param \"" + (*i)->parameterId + "\" not found. Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
		}

		std::string soapRequest;
		SonosPacket packet(_ip, frame->metaString1, frame->function1, frame->metaString2, frame->function2, soapValues);
		packet.getSoapRequest(soapRequest);
		if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: Sending SOAP request:\n" + soapRequest);
		if(_httpClient)
		{
			std::string response;
			try
			{
				_httpClient->sendRequest(soapRequest, response);
				if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + response);
				std::shared_ptr<SonosPacket> responsePacket(new SonosPacket(response));
				packetReceived(responsePacket);
				serviceMessages->setUnreach(false, true);
			}
			catch(BaseLib::HttpClientException& ex)
			{
				if(ex.responseCode() == -1) serviceMessages->setUnreach(true, false);
				return Variable::createError(-100, "Error sending value to Sonos device: " + ex.what());
			}
		}

		return parameter->convertFromPacket(valuesCentral[channel][parameter->id].data, true);
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

PParameterGroup SonosPeer::getParameterSet(int32_t channel, ParameterGroup::Type::Enum type)
{
	try
	{
		return _rpcDevice->functions.at(channel)->getParameterGroup(type);
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
	return PParameterGroup();
}

bool SonosPeer::getAllValuesHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters)
{
	try
	{
		if(channel == 1)
		{
			if(parameter->id == "IP_ADDRESS") parameter->convertToPacket(PVariable(new Variable(_ip)), valuesCentral[channel][parameter->id].data);
			else if(parameter->id == "PEER_ID") parameter->convertToPacket(PVariable(new Variable((int32_t)_peerID)), valuesCentral[channel][parameter->id].data);
			else if(parameter->id == "AV_TRANSPORT_URI" || parameter->id == "AV_TRANSPORT_URI_METADATA")
			{
				getValue(clientInfo, 1, parameter->id, true, false);
			}
			else if(parameter->id == "PLAYLISTS" || parameter->id == "FAVORITES" || parameter->id == "RADIO_FAVORITES" || parameter->id == "QUEUE_TITLES")
			{
				getValue(clientInfo, 1, parameter->id, true, false);
			}
		}
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
    return false;
}

bool SonosPeer::getParamsetHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters)
{
	try
	{
		if(channel == 1)
		{
			if(parameter->id == "IP_ADDRESS") parameter->convertToPacket(PVariable(new Variable(_ip)), valuesCentral[channel][parameter->id].data);
			else if(parameter->id == "PEER_ID") parameter->convertToPacket(PVariable(new Variable((int32_t)_peerID)), valuesCentral[channel][parameter->id].data);
			else if(parameter->id == "AV_TRANSPORT_URI" || parameter->id == "AV_TRANSPORT_URI_METADATA")
			{
				getValue(clientInfo, 1, parameter->id, true, false);
			}
			else if(parameter->id == "PLAYLISTS" || parameter->id == "FAVORITES" || parameter->id == "RADIO_FAVORITES" || parameter->id == "QUEUE_TITLES")
			{
				getValue(clientInfo, 1, parameter->id, true, false);
			}
		}
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
    return false;
}

PVariable SonosPeer::getValue(BaseLib::PRpcClientInfo clientInfo, uint32_t channel, std::string valueKey, bool requestFromDevice, bool asynchronous)
{
	try
	{
		if(serviceMessages->getUnreach()) requestFromDevice = false;
		if(channel == 1)
		{
			if(valueKey == "AV_TRANSPORT_URI" || valueKey == "AV_TRANSPORT_URI_METADATA" || valueKey == "PLAYLISTS" || valueKey == "FAVORITES" || valueKey == "RADIO_FAVORITES" || valueKey == "QUEUE_TITLES")
			{
				if(!serviceMessages->getUnreach())
				{
					//No synchronous requests when device is not reachable, so get Value and getAllValues don't block
					requestFromDevice = true;
					asynchronous = false;
				}
			}
		}
		return Peer::getValue(clientInfo, channel, valueKey, requestFromDevice, asynchronous);
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

PVariable SonosPeer::putParamset(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, PVariable variables, bool onlyPushing)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		Functions::iterator functionIterator = _rpcDevice->functions.find(channel);
		if(functionIterator == _rpcDevice->functions.end()) return Variable::createError(-2, "Unknown channel.");
		if(type == ParameterGroup::Type::none) type = ParameterGroup::Type::link;
		PParameterGroup parameterGroup = functionIterator->second->getParameterGroup(type);
		if(!parameterGroup) return Variable::createError(-3, "Unknown parameter set.");
		if(variables->structValue->empty()) return PVariable(new Variable(VariableType::tVoid));

		if(type == ParameterGroup::Type::Enum::config)
		{
			bool configChanged = false;
			for(Struct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelIterator = configCentral.find(channel);
				if(channelIterator == configCentral.end()) continue;
				std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelIterator->second.find(i->first);
				if(parameterIterator == channelIterator->second.end()) continue;
				BaseLib::Systems::RPCConfigurationParameter& parameter = parameterIterator->second;
				if(!parameter.rpcParameter) continue;
				parameter.rpcParameter->convertToPacket(i->second, parameter.data);
				if(parameter.databaseID > 0) saveParameter(parameter.databaseID, parameter.data);
				else saveParameter(0, ParameterGroup::Type::Enum::config, channel, i->first, parameter.data);
				GD::out.printInfo("Info: Parameter " + i->first + " of peer " + std::to_string(_peerID) + " and channel " + std::to_string(channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(parameter.data) + ".");
				if(parameter.rpcParameter->physical->operationType != IPhysical::OperationType::Enum::config && parameter.rpcParameter->physical->operationType != IPhysical::OperationType::Enum::configString) continue;
				configChanged = true;
			}

			if(configChanged) raiseRPCUpdateDevice(_peerID, channel, _serialNumber + ":" + std::to_string(channel), 0);
		}
		else if(type == ParameterGroup::Type::Enum::variables)
		{
			for(Struct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				setValue(clientInfo, channel, i->first, i->second, true);
			}
		}
		else
		{
			return Variable::createError(-3, "Parameter set type is not supported.");
		}
		return PVariable(new Variable(VariableType::tVoid));
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

PVariable SonosPeer::setValue(BaseLib::PRpcClientInfo clientInfo, uint32_t channel, std::string valueKey, PVariable value, bool wait)
{
	try
	{
		Peer::setValue(clientInfo, channel, valueKey, value, wait); //Ignore result, otherwise setHomegerValue might not be executed
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(valueKey.empty()) return Variable::createError(-5, "Value key is empty.");
		if(valuesCentral.find(channel) == valuesCentral.end()) return Variable::createError(-2, "Unknown channel.");
		if(setHomegearValue(channel, valueKey, value)) return PVariable(new Variable(VariableType::tVoid));
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return Variable::createError(-5, "Unknown parameter.");
		PParameter rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
		if(!rpcParameter) return Variable::createError(-5, "Unknown parameter.");
		if(rpcParameter->service)
		{
			if(channel == 0 && value->type == VariableType::tBoolean)
			{
				if(serviceMessages->set(valueKey, value->booleanValue)) return PVariable(new Variable(VariableType::tVoid));
			}
			else if(value->type == VariableType::tInteger) serviceMessages->set(valueKey, value->integerValue, channel);
		}
		if(rpcParameter->logical->type == ILogical::Type::tAction && !value->booleanValue) value->booleanValue = true;
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>());
		std::shared_ptr<std::vector<PVariable>> values(new std::vector<PVariable>());

		std::vector<uint8_t> parameterData;
		rpcParameter->convertToPacket(value, parameterData);
		value = rpcParameter->convertFromPacket(parameterData, true);

		valueKeys->push_back(valueKey);
		values->push_back(value);

		if(rpcParameter->physical->operationType == IPhysical::OperationType::Enum::command)
		{
			if(valueKey == "PLAY_FAVORITE")
			{
				BaseLib::PVariable result = playBrowsableContent(value->stringValue, "FV:0", "FAVORITES");
				if(result->errorStruct) return result;
			}
			else if(valueKey == "PLAY_PLAYLIST")
			{
				BaseLib::PVariable result = playBrowsableContent(value->stringValue, "SQ:", "PLAYLISTS");
				if(result->errorStruct) return result;
			}
			else if(valueKey == "PLAY_RADIO_FAVORITE")
			{
				BaseLib::PVariable result = playBrowsableContent(value->stringValue, "R:0/0", "RADIO_FAVORITES");
				if(result->errorStruct) return result;
			}
			else if(valueKey == "PLAY_FADE")
			{
				std::shared_ptr<SonosCentral> central(std::dynamic_pointer_cast<SonosCentral>(getCentral()));
				std::unordered_map<int32_t, std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>> peerMap = getPeers();
				std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>> peers = peerMap[1];

				int32_t volume = _currentVolume;
				execute("SetVolume", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("DesiredVolume", "0") }));
				std::vector<int32_t> peerVolumes;
				peerVolumes.reserve(peers.size());
				for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
				{
					std::shared_ptr<SonosPeer> peer = central->getPeer((*i)->id);
					if(!peer) continue;
					peerVolumes.push_back(peer->getVolume());
					peer->setVolume(0, false);
				}
				execute("Play");
				execute("RampToVolume", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("RampType", "AUTOPLAY_RAMP_TYPE"), SoapValuePair("DesiredVolume", std::to_string(volume)), SoapValuePair("ResetVolumeAfter", "false"), SoapValuePair("ProgramURI", "") }));
				int32_t j = 0;
				for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = peers.begin(); i != peers.end(); ++i, ++j)
				{
					std::shared_ptr<SonosPeer> peer = central->getPeer((*i)->id);
					if(!peer) continue;
					peer->setVolume(peerVolumes.at(j), true);
				}
			}
			else if(valueKey == "ADD_SPEAKER")
			{
				std::shared_ptr<SonosCentral> central(std::dynamic_pointer_cast<SonosCentral>(getCentral()));
				std::shared_ptr<SonosPeer> linkPeer = central->getPeer(value->integerValue);
				if(!linkPeer) return Variable::createError(-5, "Unknown remote peer.");
				central->addLink(clientInfo, _peerID, 1, linkPeer->getID(), 1, "Dynamic Sonos Link", "");
			}
			else if(valueKey == "REMOVE_SPEAKER")
			{
				std::shared_ptr<SonosCentral> central(std::dynamic_pointer_cast<SonosCentral>(getCentral()));
				std::shared_ptr<SonosPeer> linkPeer = central->getPeer(value->integerValue);
				if(!linkPeer) return Variable::createError(-5, "Unknown remote peer.");
				central->removeLink(clientInfo, _peerID, 1, linkPeer->getID(), 1);
			}
			else if(valueKey == "ADD_SPEAKER_BY_SERIAL")
			{
				std::shared_ptr<SonosCentral> central(std::dynamic_pointer_cast<SonosCentral>(getCentral()));
				std::shared_ptr<SonosPeer> linkPeer = central->getPeer(value->stringValue);
				if(!linkPeer) return Variable::createError(-5, "Unknown remote peer.");
				central->addLink(clientInfo, _peerID, 1, linkPeer->getID(), 1, "Dynamic Sonos Link", "");
			}
			else if(valueKey == "REMOVE_SPEAKER_BY_SERIAL")
			{
				std::shared_ptr<SonosCentral> central(std::dynamic_pointer_cast<SonosCentral>(getCentral()));
				std::shared_ptr<SonosPeer> linkPeer = central->getPeer(value->stringValue);
				if(!linkPeer) return Variable::createError(-5, "Unknown remote peer.");
				central->removeLink(clientInfo, _peerID, 1, linkPeer->getID(), 1);
			}
			else
			{
				if(rpcParameter->setPackets.empty()) return Variable::createError(-6, "parameter is read only");

				if(valueKey == "VOLUME") _currentVolume = value->integerValue;

				std::string setRequest = rpcParameter->setPackets.front()->id;
				if(_rpcDevice->packetsById.find(setRequest) == _rpcDevice->packetsById.end()) return Variable::createError(-6, "No frame was found for parameter " + valueKey);
				PPacket frame = _rpcDevice->packetsById[setRequest];

				std::shared_ptr<std::vector<std::pair<std::string, std::string>>> soapValues(new std::vector<std::pair<std::string, std::string>>());
				for(JsonPayloads::iterator i = frame->jsonPayloads.begin(); i != frame->jsonPayloads.end(); ++i)
				{
					if((*i)->constValueInteger > -1)
					{
						if((*i)->key.empty()) continue;
						soapValues->push_back(std::pair<std::string, std::string>((*i)->key, std::to_string((*i)->constValueInteger)));
						continue;
					}
					else if(!(*i)->constValueString.empty())
					{
						if((*i)->key.empty()) continue;
						soapValues->push_back(std::pair<std::string, std::string>((*i)->key, (*i)->constValueString));
						continue;
					}
					if((*i)->parameterId == rpcParameter->physical->groupId)
					{
						if((*i)->key.empty()) continue;
						soapValues->push_back(std::pair<std::string, std::string>((*i)->key, _binaryDecoder->decodeResponse(parameterData)->toString()));
					}
					//Search for all other parameters
					else
					{
						bool paramFound = false;
						for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator j = valuesCentral[channel].begin(); j != valuesCentral[channel].end(); ++j)
						{
							if(!j->second.rpcParameter) continue;
							if((*i)->parameterId == j->second.rpcParameter->physical->groupId)
							{
								if((*i)->key.empty()) continue;
								soapValues->push_back(std::pair<std::string, std::string>((*i)->key, _binaryDecoder->decodeResponse(j->second.data)->toString()));
								paramFound = true;
								break;
							}
						}
						if(!paramFound) GD::out.printError("Error constructing packet. param \"" + (*i)->parameterId + "\" not found. Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
					}
				}

				std::string soapRequest;
				SonosPacket packet(_ip, frame->metaString1, frame->function1, frame->metaString2, frame->function2, soapValues);
				packet.getSoapRequest(soapRequest);
				if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: Sending SOAP request:\n" + soapRequest);
				if(_httpClient)
				{
					std::string response;
					try
					{
						_httpClient->sendRequest(soapRequest, response);
						if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + response);
					}
					catch(BaseLib::HttpException& ex)
					{
						GD::out.printWarning("Warning: Error in UPnP request: " + ex.what());
						GD::out.printMessage("Request was: \n" + soapRequest);
						return Variable::createError(-100, "Error sending value to Sonos device: " + ex.what());
					}
					catch(BaseLib::HttpClientException& ex)
					{
						GD::out.printWarning("Warning: Error in UPnP request: " + ex.what());
						GD::out.printMessage("Request was: \n" + soapRequest);
						return Variable::createError(-100, "Error sending value to Sonos device: " + ex.what());
					}
					catch(BaseLib::Exception& ex)
					{
						GD::out.printWarning("Warning: Error in UPnP request: " + ex.what());
						GD::out.printMessage("Request was: \n" + soapRequest);
						return Variable::createError(-100, "Error sending value to Sonos device: " + ex.what());
					}
				}
			}
		}
		else if(rpcParameter->physical->operationType != IPhysical::OperationType::Enum::store) return Variable::createError(-6, "Only interface types \"store\" and \"command\" are supported for this device family.");

		BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[channel][valueKey];
		parameter->data = std::move(parameterData);
		if(parameter->databaseID > 0) saveParameter(parameter->databaseID, parameter->data);
		else saveParameter(0, ParameterGroup::Type::Enum::variables, channel, valueKey, parameter->data);

		raiseEvent(_peerID, channel, valueKeys, values);
		raiseRPCEvent(_peerID, channel, _serialNumber + ":" + std::to_string(channel), valueKeys, values);

		return PVariable(new Variable(VariableType::tVoid));
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
    return Variable::createError(-32500, "Unknown application error. See error log for more details.");
}

void SonosPeer::setVolume(int32_t volume, bool ramp)
{
	try
	{
		_currentVolume = volume;
		if(ramp) execute("RampToVolume", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("RampType", "AUTOPLAY_RAMP_TYPE"), SoapValuePair("DesiredVolume", std::to_string(volume)), SoapValuePair("ResetVolumeAfter", "false"), SoapValuePair("ProgramURI", "") }));
		else execute("SetVolume", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("DesiredVolume", std::to_string(volume)) }));
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

void SonosPeer::playLocalFile(std::string filename, bool now, bool unmute, int32_t volume)
{
	try
	{
		if(filename.size() < 5) return;
		std::string tempPath = GD::bl->settings.tempPath() + "sonos/";
		if(!GD::bl->io.directoryExists(tempPath))
		{
			if(GD::bl->io.createDirectory(tempPath, S_IRWXU | S_IRWXG) == false)
			{
				GD::out.printError("Error: Could not create temporary directory \"" + tempPath + '"');
				return;
			}
		}

		if(now)
		{
			execute("GetPositionInfo");
			execute("GetVolume");
			execute("GetMute");
			execute("GetTransportInfo");
		}

		std::string playlistFilename = filename.substr(0, filename.size() - 4) + ".m3u";
		BaseLib::HelperFunctions::stringReplace(playlistFilename, "/", "_");
		std::string playlistContent = "#EXTM3U\n#EXTINF:0,<Homegear><TTS><TTS>\nhttp://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + '/' + filename + '\n';
		std::string playlistFilepath = tempPath + playlistFilename;
		BaseLib::Io::writeFile(playlistFilepath, playlistContent);
		playlistFilename = BaseLib::Http::encodeURL(playlistFilename);

		std::string silence2sPlaylistFilename = "silence_2s.m3u";
		playlistContent = "#EXTM3U\n#EXTINF:0,<Homegear><TTS><TTS>\nhttp://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + "/Silence_2s.mp3\n";
		std::string silencePlaylistFilepath = tempPath + silence2sPlaylistFilename;
		BaseLib::Io::writeFile(silencePlaylistFilepath, playlistContent);
		silence2sPlaylistFilename = BaseLib::Http::encodeURL(silence2sPlaylistFilename);

		std::string silence10sPlaylistFilename = "silence_10s.m3u";
		playlistContent = "#EXTM3U\n#EXTINF:0,<Homegear><TTS><TTS>\nhttp://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + "/Silence_10s.mp3\n";
		silencePlaylistFilepath = tempPath + silence10sPlaylistFilename;
		BaseLib::Io::writeFile(silencePlaylistFilepath, playlistContent);
		silence10sPlaylistFilename = BaseLib::Http::encodeURL(silence10sPlaylistFilename);

		std::string rinconId;
		std::string currentTransportUri;
		std::string currentTransportUriMetadata;
		std::string transportState;
		int32_t volumeState = -1;
		std::string playmodeState;
		bool muteState = false;
		int32_t trackNumberState = -1;
		std::string seekTimeState;
		bool setQueue = false;

		std::shared_ptr<SonosCentral> central(std::dynamic_pointer_cast<SonosCentral>(getCentral()));
		std::unordered_map<int32_t, std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>> peerMap = getPeers();
		std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>> peers = peerMap[1];

		std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
		if(channelOneIterator == valuesCentral.end())
		{
			GD::out.printError("Error: Channel 1 not found.");
			return;
		}
		if(now)
		{
			execute("GetMediaInfo");

			std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("AV_TRANSPORT_URI");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) currentTransportUri = variable->stringValue;
				if(currentTransportUri.compare(0, 14, "x-rincon-queue") != 0)
				{
					GD::out.printInfo("Info (peer " + std::to_string(_peerID) + "): Currently playing non-rincon queue (e. g. radio).");
					setQueue = true;

					parameterIterator = channelOneIterator->second.find("AV_TRANSPORT_METADATA");
					if(parameterIterator != channelOneIterator->second.end())
					{
						PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
						if(variable) currentTransportUriMetadata = variable->stringValue;
					}
				}
			}

			parameterIterator = channelOneIterator->second.find("VOLUME");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) volumeState = variable->integerValue;
			}

			parameterIterator = channelOneIterator->second.find("MUTE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) muteState = (variable->stringValue == "1");
			}

			parameterIterator = channelOneIterator->second.find("CURRENT_TRACK");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) trackNumberState = variable->integerValue;
			}

			parameterIterator = channelOneIterator->second.find("CURRENT_TRACK_RELATIVE_TIME");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) seekTimeState = variable->stringValue;
			}

			parameterIterator = channelOneIterator->second.find("TRANSPORT_STATE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) transportState = variable->stringValue;
			}

			parameterIterator = channelOneIterator->second.find("CURRENT_PLAY_MODE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) playmodeState = variable->stringValue;
			}

			parameterIterator = channelOneIterator->second.find("ID");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) rinconId = variable->stringValue;
			}

			execute("Pause", true);

			if(unmute && muteState) execute("SetMute", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("DesiredMute", "0") }));
			if(volume > 0)
			{
				execute("SetVolume", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("DesiredVolume", std::to_string(volume)) }));
				for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
				{
					std::shared_ptr<SonosPeer> peer = central->getPeer((*i)->id);
					if(!peer) continue;
					peer->setVolume(volume);
				}
			}
		}

		std::string playlistUri = "http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + '/' + silence10sPlaylistFilename;
		execute("AddURIToQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("EnqueuedURI", playlistUri), SoapValuePair("EnqueuedURIMetaData", ""), SoapValuePair("DesiredFirstTrackNumberEnqueued", "1"), SoapValuePair("EnqueueAsNext", "1") }));

		playlistUri = "http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + '/' + playlistFilename;
		execute("AddURIToQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("EnqueuedURI", playlistUri), SoapValuePair("EnqueuedURIMetaData", ""), SoapValuePair("DesiredFirstTrackNumberEnqueued", "1"), SoapValuePair("EnqueueAsNext", "1") }));

		playlistUri = "http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + '/' + silence2sPlaylistFilename;
		execute("AddURIToQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("EnqueuedURI", playlistUri), SoapValuePair("EnqueuedURIMetaData", ""), SoapValuePair("DesiredFirstTrackNumberEnqueued", "1"), SoapValuePair("EnqueueAsNext", "1") }));

		if(now)
		{
			execute("SetAVTransportURI", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("CurrentURI", "x-rincon-queue:" + rinconId + "#0"), SoapValuePair("CurrentURIMetaData", "") }));

			//std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
			//std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("FIRST_TRACK_NUMBER_ENQUEUED");
			/*if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) trackNumber = variable->integerValue;
			}*/
			_currentTrack = 1;

			execute("Seek", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Unit", "TRACK_NR"), SoapValuePair("Target", std::to_string(1)) }));
			execute("Play");

			std::this_thread::sleep_for(std::chrono::milliseconds(2000));

			while(!serviceMessages->getUnreach() && (_currentTrack == 1 || _currentTrack == 2))
			{
				for(int32_t i = 0; i < 50; i++)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					if(_currentTrack != 1 && _currentTrack != 2) break;
				}

				execute("GetPositionInfo");

				std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("CURRENT_TRACK");
				if(parameterIterator != channelOneIterator->second.end())
				{
					PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(!variable || (variable->integerValue != 1 && variable->integerValue != 2)) break;
				}
				else break;

				execute("GetTransportInfo");

				parameterIterator = channelOneIterator->second.find("TRANSPORT_STATE");
				if(parameterIterator != channelOneIterator->second.end())
				{
					PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(!variable || (variable->stringValue != "PLAYING" && variable->stringValue != "TRANSITIONING")) break;
				}
				else break;
			}

			//Pause often causes errors at this point
			execute("SetVolume", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("DesiredVolume", "0") }));
			for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				std::shared_ptr<SonosPeer> peer = central->getPeer((*i)->id);
				if(!peer) continue;
				peer->setVolume(0);
			}
			if(muteState) execute("SetMute", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("DesiredMute", std::to_string((int32_t)muteState)) }));
			execute("RemoveTrackFromQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("ObjectID", "Q:0/" + std::to_string(1)) }));
			execute("RemoveTrackFromQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("ObjectID", "Q:0/" + std::to_string(1)) }));
			execute("RemoveTrackFromQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("ObjectID", "Q:0/" + std::to_string(1)) }));
			if(trackNumberState > 0) execute("Seek", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Unit", "TRACK_NR"), SoapValuePair("Target", std::to_string(trackNumberState)) }));
			if(!seekTimeState.empty()) execute("Seek", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Unit", "REL_TIME"), SoapValuePair("Target", seekTimeState) }));

			if(setQueue) execute("SetAVTransportURI", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("CurrentURI", currentTransportUri), SoapValuePair("CurrentURIMetaData", currentTransportUriMetadata) }));

			if(transportState == "PLAYING")
			{
				GD::out.printInfo("Info (peer " + std::to_string(_peerID) + "): Resuming playback, because TRANSPORT_STATE was PLAYING.");
				if(setQueue) execute("Play");

				for(int32_t i = 0; i < 10; i++)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));

					std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("TRANSPORT_STATE");
					if(parameterIterator != channelOneIterator->second.end())
					{
						PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
						if(!variable || (variable->stringValue != "TRANSITIONING")) break;
					}
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				execute("RampToVolume", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("RampType", "AUTOPLAY_RAMP_TYPE"), SoapValuePair("DesiredVolume", std::to_string(volumeState)), SoapValuePair("ResetVolumeAfter", "false"), SoapValuePair("ProgramURI", "") }));
				for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
				{
					std::shared_ptr<SonosPeer> peer = central->getPeer((*i)->id);
					if(!peer) continue;
					peer->setVolume(volumeState, true);
				}
			}
			else
			{
				GD::out.printInfo("Info (peer " + std::to_string(_peerID) + "): Not resuming playback, because TRANSPORT_STATE was " + transportState + ".");
				execute("Pause");
				execute("SetVolume", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("DesiredVolume", std::to_string(volumeState)) }));
				for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
				{
					std::shared_ptr<SonosPeer> peer = central->getPeer((*i)->id);
					if(!peer) continue;
					peer->setVolume(volumeState);
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, std::string(ex.what()) + " Filename: " + filename);
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what() + " Filename: " + filename);
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Filename: " + filename);
    }
}

bool SonosPeer::setHomegearValue(uint32_t channel, std::string valueKey, PVariable value)
{
	try
	{
		if(valueKey == "PLAY_TTS")
		{
			if(value->stringValue.empty()) return true;
			std::string ttsProgram = GD::physicalInterface->ttsProgram();
			if(ttsProgram.empty())
			{
				GD::out.printError("Error: No program to generate TTS audio file specified in sonos.conf");
				return true;
			}

			bool unmute = true;
			int32_t volume = -1;
			std::string language;

			std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
			if(channelOneIterator == valuesCentral.end())
			{
				GD::out.printError("Error: Channel 1 not found.");
				return true;
			}

			std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("PLAY_TTS_UNMUTE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) unmute = variable->booleanValue;
			}

			parameterIterator = channelOneIterator->second.find("PLAY_TTS_VOLUME");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) volume = variable->integerValue;
			}

			parameterIterator = channelOneIterator->second.find("PLAY_TTS_LANGUAGE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) language = variable->stringValue;
				if(!BaseLib::HelperFunctions::isAlphaNumeric(language))
				{
					GD::out.printError("Error: Language is not alphanumeric.");
					language = "en";
				}
			}

			std::string audioPath = GD::bl->settings.tempPath() + "sonos/";
			std::string filename;
			BaseLib::HelperFunctions::stringReplace(value->stringValue, "\"", "");
			std::string execPath = ttsProgram + ' ' + language + " \"" + value->stringValue + "\"";
			if(BaseLib::HelperFunctions::exec(execPath, filename) != 0)
			{
				GD::out.printError("Error: Error executing program to generate TTS audio file: \"" + ttsProgram + ' ' + language + ' ' + value->stringValue + "\"");
				return true;
			}
			BaseLib::HelperFunctions::trim(filename);
			if(!BaseLib::Io::fileExists(filename))
			{
				GD::out.printError("Error: Error executing program to generate TTS audio file: File not found. Output needs to be the full path to the TTS audio file, but was: \"" + filename + "\"");
				return true;
			}
			if(filename.size() <= audioPath.size() || filename.compare(0, audioPath.size(), audioPath) != 0)
			{
				GD::out.printError("Error: Error executing program to generate TTS audio file. Output needs to be the full path to the TTS audio file and the file needs to be within \"" + audioPath + "\". Returned path was: \"" + filename + "\"");
				return true;
			}
			filename = filename.substr(audioPath.size());

			playLocalFile(filename, true, unmute, volume);

			return true;
		}
		else if(valueKey == "PLAY_AUDIO_FILE")
		{
			if(value->stringValue.empty()) return true;

			bool unmute = true;
			int32_t volume = -1;

			std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
			if(channelOneIterator == valuesCentral.end())
			{
				GD::out.printError("Error: Channel 1 not found.");
				return true;
			}

			std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("PLAY_AUDIO_FILE_UNMUTE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) unmute = variable->booleanValue;
			}

			parameterIterator = channelOneIterator->second.find("PLAY_AUDIO_FILE_VOLUME");
			if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) volume = variable->integerValue;
			}

			std::string audioPath = GD::dataPath;
			if(!BaseLib::Io::fileExists(audioPath + value->stringValue))
			{
				GD::out.printError("Error: Can't stream audio file \"" + audioPath + value->stringValue + "\". File not found.");
				return true;
			}

			playLocalFile(value->stringValue, true, unmute, volume);

			return true;
		}
		else if(valueKey == "ENQUEUE_AUDIO_FILE")
		{
			if(value->stringValue.empty()) return true;

			std::string audioPath = GD::physicalInterface->dataPath();
			if(!BaseLib::Io::fileExists(audioPath + value->stringValue))
			{
				GD::out.printError("Error: Can't stream audio file \"" + audioPath + value->stringValue + "\". File not found.");
				return true;
			}

			playLocalFile(value->stringValue, false, false, -1);

			return true;
		}
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
	return false;
}

}
