/* Copyright 2013-2019 Homegear GmbH
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

#include <homegear-base/Managers/ProcessManager.h>

#include "sys/wait.h"

#include <iomanip>

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
}

void SonosPeer::init()
{
    _shuttingDown = false;
    _getOneMorePositionInfo = true;
    _isMaster = false;
    _isStream = false;

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
		std::string settingName = "readtimeout";
		BaseLib::Systems::FamilySettings::PFamilySetting readTimeoutSetting = GD::family->getFamilySetting(settingName);
		int32_t readTimeout = 10000;
		if(readTimeoutSetting) readTimeout = readTimeoutSetting->integerValue;
		if(readTimeout < 1 || readTimeout > 120000) readTimeout = 10000;
		_httpClient.reset(new BaseLib::HttpClient(GD::bl, _ip, 1400, false));
		_httpClient->setTimeout(readTimeout);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
		std::vector<uint8_t> parameterData = valuesCentral[1]["ID"].getBinaryData();
		return parameter->convertFromPacket(parameterData, Role(), false)->stringValue;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return "";
}

void SonosPeer::setRinconId(std::string value)
{
	try
	{
		BaseLib::Systems::RpcConfigurationParameter& configParameter = valuesCentral[1]["ID"];
		if(!configParameter.rpcParameter) return;
		std::vector<uint8_t> parameterData;
		configParameter.rpcParameter->convertToPacket(PVariable(new Variable(value)), Role(), parameterData);
		if(configParameter.equals(parameterData)) return;
		configParameter.setBinaryData(parameterData);
		if(configParameter.databaseId > 0) saveParameter(configParameter.databaseId, parameterData);
		else saveParameter(0, ParameterGroup::Type::Enum::variables, 1, "ID", parameterData);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

void SonosPeer::setRoomName(std::string value, bool broadCastEvent)
{
	try
	{
		BaseLib::Systems::RpcConfigurationParameter& configParameter = valuesCentral[1]["ROOMNAME"];
		if(!configParameter.rpcParameter) return;
		PVariable variable(new Variable(value));
		std::vector<uint8_t> parameterData;
		configParameter.rpcParameter->convertToPacket(variable, Role(), parameterData);
		if(configParameter.equals(parameterData)) return;
		configParameter.setBinaryData(parameterData);
		if(configParameter.databaseId > 0) saveParameter(configParameter.databaseId, parameterData);
		else saveParameter(0, ParameterGroup::Type::Enum::variables, 1, "ROOMNAME", parameterData);

		if(broadCastEvent)
		{
			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{ "ROOMNAME" });
			std::shared_ptr<std::vector<PVariable>> values(new std::vector<PVariable>{ variable });
            std::string eventSource = "device-" + std::to_string(_peerID);
			std::string address = _serialNumber + ":1";
            raiseEvent(eventSource, _peerID, 1, valueKeys, values);
			raiseRPCEvent(eventSource, _peerID, 1, address, valueKeys, values);
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

void SonosPeer::worker()
{
	try
	{
		if(_shuttingDown || deleting) return;
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>());
		std::shared_ptr<std::vector<PVariable>> rpcValues(new std::vector<PVariable>());
		if( BaseLib::HelperFunctions::getTimeSeconds() - _lastPositionInfo > 5 && !serviceMessages->getUnreach())
		{
			_lastPositionInfo = BaseLib::HelperFunctions::getTimeSeconds();
			//Get position info if TRANSPORT_STATE is PLAYING
			std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator channelIterator = valuesCentral.find(1);
			if(channelIterator != valuesCentral.end())
			{
				std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator parameterIterator = channelIterator->second.find("TRANSPORT_STATE");
				if(parameterIterator != channelIterator->second.end())
				{
					std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
					PVariable transportState = _binaryDecoder->decodeResponse(parameterData);
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
				for(uint32_t i = 0; i < subscriptionPackets.size(); i++)
				{
					BaseLib::Http response;
					try
					{
						_httpClient->sendRequest(subscriptionPackets.at(i), response, true);
						std::string stringResponse(response.getContent().data(), response.getContentSize());
						if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + stringResponse);
						if(response.getHeader().responseCode < 200 || response.getHeader().responseCode > 299)
						{
							GD::out.printWarning("Warning: Error calling SUBSCRIBE (" + std::to_string(i) + ") on Sonos device: Response code was: " + std::to_string(response.getHeader().responseCode));
							if(response.getHeader().responseCode == -1)
							{
								//serviceMessages->setUnreach(true, false);
								break;
							}
						}
						serviceMessages->setUnreach(false, true);
					}
					catch(const BaseLib::HttpClientException& ex)
					{
						GD::out.printWarning("Warning: Error calling SUBSCRIBE (" + std::to_string(i) + ") on Sonos device: " + ex.what());
						if(ex.responseCode() == -1)
						{
							serviceMessages->setUnreach(true, false);
							break;
						}
					}
					catch(const std::exception& ex)
					{
						GD::out.printWarning("Warning: Error calling SUBSCRIBE (" + std::to_string(i) + ") on Sonos device: " + ex.what());
						break;
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
}

std::string SonosPeer::printConfig()
{
	try
	{
		std::ostringstream stringStream;
		stringStream << "MASTER" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator i = configCentral.begin(); i != configCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t[" << j->first << "]: ";
				if(!j->second.rpcParameter) stringStream << "(No RPC parameter) ";
				std::vector<uint8_t> parameterData = j->second.getBinaryData();
				for(std::vector<uint8_t>::const_iterator k = parameterData.begin(); k != parameterData.end(); ++k)
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
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator i = valuesCentral.begin(); i != valuesCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t[" << j->first << "]: ";
				if(!j->second.rpcParameter) stringStream << "(No RPC parameter) ";
				std::vector<uint8_t> parameterData = j->second.getBinaryData();
				for(std::vector<uint8_t>::const_iterator k = parameterData.begin(); k != parameterData.end(); ++k)
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
    return "";
}


void SonosPeer::loadVariables(BaseLib::Systems::ICentral* central, std::shared_ptr<BaseLib::Database::DataTable>& rows)
{
	try
	{
		std::string settingName = "readtimeout";
		BaseLib::Systems::FamilySettings::PFamilySetting readTimeoutSetting = GD::family->getFamilySetting(settingName);
		int32_t readTimeout = 10000;
		if(readTimeoutSetting) readTimeout = readTimeoutSetting->integerValue;
		if(readTimeout < 1 || readTimeout > 120000) readTimeout = 10000;

		if(!rows) rows = _bl->db->getPeerVariables(_peerID);
		Peer::loadVariables(central, rows);
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 12:
				unserializePeers(row->second.at(5)->binaryValue);
				break;
			}
		}
		_httpClient.reset(new BaseLib::HttpClient(GD::bl, _ip, 1400, false));
		_httpClient->setTimeout(readTimeout);
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
			GD::out.printError("Error loading Sonos peer " + std::to_string(_peerID) + ": Device type not found: 0x" + BaseLib::HelperFunctions::getHexString(_deviceType) + " Firmware version: " + std::to_string(_firmwareVersion));
			return false;
		}
		initializeTypeString();
		std::string entry;
		loadConfig();
		initializeCentralConfig();

		serviceMessages.reset(new BaseLib::Systems::ServiceMessages(_bl, _peerID, _serialNumber, this));
		serviceMessages->load();

		std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
		if(channelOneIterator != valuesCentral.end())
		{
			auto parameterIterator = channelOneIterator->second.find("VOLUME");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) _currentVolume = variable->integerValue;
			}

			parameterIterator = channelOneIterator->second.find("IS_MASTER");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) _isMaster = variable->booleanValue;
			}

            parameterIterator = channelOneIterator->second.find("IS_STREAM");
            if(parameterIterator != channelOneIterator->second.end())
            {
                std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
                PVariable variable = _binaryDecoder->decodeResponse(parameterData);
                if(variable) _isStream = variable->booleanValue;
            }
		}

		return true;
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return false;
}

void SonosPeer::saveVariables()
{
	try
	{
		if(_peerID == 0) return;
		Peer::saveVariables();
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
						PVariable data = (*k)->convertFromPacket(encodedData, Role(), true);
						(*k)->convertToPacket(data, Role(), currentFrameValues.values[(*k)->id].value);
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

					BaseLib::Systems::RpcConfigurationParameter& parameter = valuesCentral[*j][i->first];
					if(parameter.equals(i->second.value) && i->first != "AV_TRANSPORT_URI") continue;

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

					if(parameter.rpcParameter)
					{
						//Process service messages
						if(parameter.rpcParameter->service && !i->second.value.empty())
						{
							if(parameter.rpcParameter->logical->type == ILogical::Type::Enum::tEnum)
							{
								serviceMessages->set(i->first, i->second.value.at(i->second.value.size() - 1), *j);
							}
							else if(parameter.rpcParameter->logical->type == ILogical::Type::Enum::tBoolean)
							{
								serviceMessages->set(i->first, (bool)i->second.value.at(i->second.value.size() - 1));
							}
						}

						PVariable value = parameter.rpcParameter->convertFromPacket(i->second.value, parameter.mainRole(), true);
						if(i->first == "CURRENT_TRACK") _currentTrack = value->integerValue;
						if(i->first == "CURRENT_ALBUM_ART")
						{
                            std::string artPath;
                            BaseLib::Html::unescapeHtmlEntities(value->stringValue, artPath);
							value->stringValue = "http://" + _ip + ":1400" + artPath;
                            parameter.rpcParameter->convertToPacket(value, parameter.mainRole(), i->second.value);
						}
						else if(i->first == "NEXT_ALBUM_ART")
						{
                            std::string artPath;
                            BaseLib::Html::unescapeHtmlEntities(value->stringValue, artPath);
                            value->stringValue = "http://" + _ip + ":1400" + artPath;
                            parameter.rpcParameter->convertToPacket(value, parameter.mainRole(), i->second.value);
						}
						if(i->first == "CURRENT_TRACK_URI" && value->stringValue.empty())
						{
							//Clear current track information when uri is empty
							std::vector<uint8_t> emptyData;

							BaseLib::Systems::RpcConfigurationParameter& parameter2 = valuesCentral[1]["CURRENT_TITLE"];
							parameter2.setBinaryData(emptyData);
							if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, emptyData);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "CURRENT_TITLE", emptyData);
							valueKeys[1]->push_back("CURRENT_TITLE");
							rpcValues[1]->push_back(value);

							parameter2 = valuesCentral[1]["CURRENT_ALBUM"];
							parameter2.setBinaryData(emptyData);
							if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, emptyData);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "CURRENT_ALBUM", emptyData);
							valueKeys[1]->push_back("CURRENT_ALBUM");
							rpcValues[1]->push_back(value);

							parameter2 = valuesCentral[1]["CURRENT_ARTIST"];
							parameter2.setBinaryData(emptyData);
							if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, emptyData);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "CURRENT_ARTIST", emptyData);
							valueKeys[1]->push_back("CURRENT_ARTIST");
							rpcValues[1]->push_back(value);

							parameter2 = valuesCentral[1]["CURRENT_ALBUM_ART"];
							parameter2.setBinaryData(emptyData);
							if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, emptyData);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "CURRENT_ALBUM_ART", emptyData);
							valueKeys[1]->push_back("CURRENT_ALBUM_ART");
							rpcValues[1]->push_back(value);
						}
						else if(i->first == "NEXT_TRACK_URI" && value->stringValue.empty())
						{
							//Clear next track information when uri is empty
							std::vector<uint8_t> emptyData;

							BaseLib::Systems::RpcConfigurationParameter& parameter2 = valuesCentral[1]["NEXT_TITLE"];
							parameter2.setBinaryData(emptyData);
							if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, emptyData);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "NEXT_TITLE", emptyData);
							valueKeys[1]->push_back("NEXT_TITLE");
							rpcValues[1]->push_back(value);

							parameter2 = valuesCentral[1]["NEXT_ALBUM"];
							parameter2.setBinaryData(emptyData);
							if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, emptyData);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "NEXT_ALBUM", emptyData);
							valueKeys[1]->push_back("NEXT_ALBUM");
							rpcValues[1]->push_back(value);

							parameter2 = valuesCentral[1]["NEXT_ARTIST"];
							parameter2.setBinaryData(emptyData);
							if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, emptyData);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "NEXT_ARTIST", emptyData);
							valueKeys[1]->push_back("NEXT_ARTIST");
							rpcValues[1]->push_back(value);

							parameter2 = valuesCentral[1]["NEXT_ALBUM_ART"];
							parameter2.setBinaryData(emptyData);
							if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, emptyData);
							else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "NEXT_ALBUM_ART", emptyData);
							valueKeys[1]->push_back("NEXT_ALBUM_ART");
							rpcValues[1]->push_back(value);
						}
						else if(i->first == "AV_TRANSPORT_URI")
						{
							std::shared_ptr<SonosCentral> central = std::dynamic_pointer_cast<SonosCentral>(getCentral());
							std::vector<uint8_t> transportParameterData = parameter.getBinaryData();
							BaseLib::PVariable oldValue = parameter.rpcParameter->convertFromPacket(transportParameterData, parameter.mainRole(), true);

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

                            //Update IS_MASTER and MASTER_ID
                            {
                                BaseLib::Systems::RpcConfigurationParameter& parameter2 = valuesCentral[1]["IS_MASTER"];
                                if(parameter2.rpcParameter)
                                {
                                    std::vector<uint8_t> parameterData = parameter2.getBinaryData();
                                    _isMaster = value->stringValue.empty() || (value->stringValue.size() >= 9 && value->stringValue.compare(0, 9, "x-rincon:") != 0);
                                    BaseLib::PVariable isMaster = std::make_shared<BaseLib::Variable>(_isMaster);
                                    if(parameterData.empty() || (bool) parameterData.back() != _isMaster)
                                    {
                                        parameter2.rpcParameter->convertToPacket(isMaster, parameter2.mainRole(), parameterData);
                                        parameter2.setBinaryData(parameterData);
                                        auto bla = parameter2.getBinaryData();
                                        if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, parameterData);
                                        else saveParameter(0, ParameterGroup::Type::Enum::variables, 1, "IS_MASTER", parameterData);
                                        valueKeys[1]->push_back("IS_MASTER");
                                        rpcValues[1]->push_back(isMaster);

                                        //{{{ MASTER_ID
                                        BaseLib::PVariable masterId;
                                        if(!_isMaster)
                                        {
                                            std::unordered_map<int32_t, std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>> peerMap = getPeers();
                                            for(auto basicPeer : peerMap[1])
                                            {
                                                if(!basicPeer->isSender) continue;
                                                masterId = std::make_shared<BaseLib::Variable>((int32_t)basicPeer->id);
                                                break;
                                            }
                                        }
                                        if(!masterId) masterId = std::make_shared<BaseLib::Variable>(0);

                                        BaseLib::Systems::RpcConfigurationParameter& parameter3 = valuesCentral[1]["MASTER_ID"];
                                        if(parameter3.rpcParameter)
                                        {
                                            std::vector<uint8_t> parameterData2;
                                            parameter3.rpcParameter->convertToPacket(masterId, parameter3.mainRole(), parameterData2);
                                            parameter3.setBinaryData(parameterData2);
                                            if(parameter3.databaseId > 0) saveParameter(parameter3.databaseId, parameterData2);
                                            else saveParameter(0, ParameterGroup::Type::Enum::variables, 1, "MASTER_ID", parameterData2);
                                            valueKeys[1]->push_back("MASTER_ID");
                                            rpcValues[1]->push_back(masterId);
                                        }
                                        //}}}
                                    }
                                }
                            }

							if(value->stringValue.compare(0, 18, "x-sonosapi-stream:") == 0)
							{
                                _isStream = true;
								//Delete track information for radio
								std::vector<uint8_t> emptyData;

								PVariable value = std::make_shared<BaseLib::Variable>(VariableType::tString);

								{
									BaseLib::Systems::RpcConfigurationParameter& parameter3 = valuesCentral[1]["AV_TRANSPORT_TITLE"];
									BaseLib::Systems::RpcConfigurationParameter& parameter2 = valuesCentral[1]["CURRENT_ALBUM"];
									parameter2.setBinaryData(parameter3.getBinaryDataReference());
									if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, parameter2.getBinaryDataReference());
									else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "CURRENT_ALBUM", parameter2.getBinaryDataReference());
									valueKeys[1]->push_back("CURRENT_ALBUM");
									rpcValues[1]->push_back(value);
								}

								{
									BaseLib::Systems::RpcConfigurationParameter& parameter2 = valuesCentral[1]["CURRENT_ARTIST"];
									parameter2.setBinaryData(emptyData);
									if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, emptyData);
									else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "CURRENT_ARTIST", emptyData);
									valueKeys[1]->push_back("CURRENT_ARTIST");
									rpcValues[1]->push_back(value);
								}

								{
									BaseLib::Systems::RpcConfigurationParameter& parameter2 = valuesCentral[1]["CURRENT_ALBUM_ART"];
									parameter2.setBinaryData(emptyData);
									if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, emptyData);
									else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "CURRENT_ALBUM_ART", emptyData);
									valueKeys[1]->push_back("CURRENT_ALBUM_ART");
									rpcValues[1]->push_back(value);
								}

								{
									BaseLib::Systems::RpcConfigurationParameter& parameter3 = valuesCentral[1]["CURRENT_TRACK_STREAM_CONTENT"];
									BaseLib::Systems::RpcConfigurationParameter& parameter2 = valuesCentral[1]["CURRENT_TITLE"];
									parameter2.setBinaryData(parameter3.getBinaryDataReference());
									if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, parameter2.getBinaryDataReference());
									else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "CURRENT_TITLE", parameter2.getBinaryDataReference());
									valueKeys[1]->push_back("CURRENT_TITLE");
									rpcValues[1]->push_back(value);
								}
							}
                            else _isStream = false;

                            //{{{ Update IS_STREAM
								BaseLib::Systems::RpcConfigurationParameter& parameter3 = valuesCentral[1]["IS_STREAM"];
								parameter3 = valuesCentral[1]["IS_STREAM"];
                                if(parameter3.rpcParameter)
								{
									std::vector<uint8_t> parameterData = parameter3.getBinaryData();
									if(parameterData.empty() || (bool) parameterData.back() != _isStream)
									{
										BaseLib::PVariable isStream(new BaseLib::Variable(_isStream));
										parameter3.rpcParameter->convertToPacket(isStream, parameter3.mainRole(), parameterData);
										parameter3.setBinaryData(parameterData);
										if(parameter3.databaseId > 0) saveParameter(parameter3.databaseId, parameterData);
										else saveParameter(0, ParameterGroup::Type::Enum::variables, 1, "IS_STREAM", parameterData);
										valueKeys[1]->push_back("IS_STREAM");
										rpcValues[1]->push_back(isStream);
									}
								}
                            //}}}
						}
						else if(i->first == "VOLUME") _currentVolume = value->integerValue;
						else if(_isStream)
						{
							if(i->first == "CURRENT_TRACK_STREAM_CONTENT")
							{
								BaseLib::Systems::RpcConfigurationParameter& parameter2 = valuesCentral[1]["CURRENT_TITLE"];
								if(parameter2.rpcParameter)
								{
									std::vector<uint8_t> binaryData;
									parameter2.rpcParameter->convertToPacket(value, parameter2.mainRole(), binaryData);
									parameter2.setBinaryData(binaryData);
									if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, binaryData);
									else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "CURRENT_TITLE", binaryData);
									valueKeys[1]->push_back("CURRENT_TITLE");
									rpcValues[1]->push_back(value);
								}
							}
							else if(i->first == "AV_TRANSPORT_TITLE")
							{
								BaseLib::Systems::RpcConfigurationParameter& parameter2 = valuesCentral[1]["CURRENT_ALBUM"];
								if(parameter2.rpcParameter)
								{
									std::vector<uint8_t> binaryData;
									parameter2.rpcParameter->convertToPacket(value, parameter2.mainRole(), binaryData);
									parameter2.setBinaryData(binaryData);
									if(parameter2.databaseId > 0) saveParameter(parameter2.databaseId, binaryData);
									else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, "CURRENT_ALBUM", binaryData);
									valueKeys[1]->push_back("CURRENT_ALBUM");
									rpcValues[1]->push_back(value);
								}
							}
							else if(i->first == "CURRENT_TITLE") continue; //Ignore
							else if(i->first == "CURRENT_ALBUM") continue; //Ignore
						}

						parameter.setBinaryData(i->second.value);
						if(parameter.databaseId > 0) saveParameter(parameter.databaseId, i->second.value);
						else saveParameter(0, ParameterGroup::Type::Enum::variables, *j, i->first, i->second.value);
						if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + i->first + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(*j) + " was set.");

						valueKeys[*j]->push_back(i->first);
						rpcValues[*j]->push_back(value);
					}
				}
			}
		}

		if(!rpcValues.empty())
		{
			for(std::map<uint32_t, std::shared_ptr<std::vector<std::string>>>::iterator j = valueKeys.begin(); j != valueKeys.end(); ++j)
			{
				if(j->second->empty()) continue;

                std::string eventSource = "device-" + std::to_string(_peerID);
                std::string address(_serialNumber + ":" + std::to_string(j->first));
                raiseEvent(eventSource, _peerID, j->first, j->second, rpcValues.at(j->first));
                raiseRPCEvent(eventSource, _peerID, j->first, address, j->second, rpcValues.at(j->first));
			}
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

bool SonosPeer::sendSoapRequest(std::string& request, bool ignoreErrors)
{
	try
	{
		if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: Sending SOAP request:\n" + request);
		if(_httpClient)
		{
			BaseLib::Http response;
			try
			{
				_httpClient->sendRequest(request, response);
				std::string stringResponse(response.getContent().data(), response.getContentSize());
				if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + stringResponse);
				if(response.getHeader().responseCode < 200 || response.getHeader().responseCode > 299)
				{
					if(ignoreErrors) return false;
					GD::out.printWarning("Warning: Error in UPnP request: Response code was: " + std::to_string(response.getHeader().responseCode));
					GD::out.printMessage("Request was: \n" + request);
					//if(response.getHeader().responseCode == -1) serviceMessages->setUnreach(true, false);
				}
				else
				{
					std::shared_ptr<SonosPacket> responsePacket(new SonosPacket(stringResponse));
					packetReceived(responsePacket);
					serviceMessages->setUnreach(false, true);
					return true;
				}
			}
			catch(const BaseLib::HttpClientException& ex)
			{
				if(ignoreErrors) return false;
				GD::out.printWarning("Warning: Error in UPnP request: " + std::string(ex.what()));
				GD::out.printMessage("Request was: \n" + request);
				if(ex.responseCode() == -1) serviceMessages->setUnreach(true, false);
			}
			catch(const std::exception& ex)
			{
				if(ignoreErrors) return false;
				GD::out.printWarning("Warning: Error in UPnP request: " + std::string(ex.what()));
				GD::out.printMessage("Request was: \n" + request);
				serviceMessages->setUnreach(true, false);
			}
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return false;
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
}

bool SonosPeer::execute(std::string functionName, PSoapValues soapValues, bool ignoreErrors)
{
	try
	{
		UpnpFunctions::iterator functionEntry = _upnpFunctions.find(functionName);
		if(functionEntry == _upnpFunctions.end())
		{
			GD::out.printError("Error: Tried to execute unknown function: " + functionName);
			return false;
		}
		std::string soapRequest;
		std::string headerSoapRequest = functionEntry->second.service() + '#' + functionName;
		SonosPacket packet(_ip, functionEntry->second.path(), headerSoapRequest, functionEntry->second.service(), functionName, soapValues);
		packet.getSoapRequest(soapRequest);
		return sendSoapRequest(soapRequest, ignoreErrors);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return false;
}

PVariable SonosPeer::playBrowsableContent(std::string& title, std::string browseId, std::string listVariable)
{
	try
	{
		std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
		if(channelOneIterator == valuesCentral.end())
		{
			GD::out.printError("Error: Channel 1 not found.");
			return Variable::createError(-32500, "Channel 1 not found.");
		}

		execute("Browse", PSoapValues(new SoapValues{ SoapValuePair("ObjectID", browseId), SoapValuePair("BrowseFlag", "BrowseDirectChildren"), SoapValuePair("Filter", ""), SoapValuePair("StartingIndex", "0"), SoapValuePair("RequestedCount", "0"), SoapValuePair("SortCriteria", "") }));

		PVariable entries;
		std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find(listVariable);
		if(parameterIterator != channelOneIterator->second.end())
		{
			std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
			entries = _binaryDecoder->decodeResponse(parameterData);
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
			std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
			PVariable variable = _binaryDecoder->decodeResponse(parameterData);
			if(variable) rinconId = variable->stringValue;
		}
		if(rinconId.empty()) return Variable::createError(-32500, "Rincon id could not be decoded.");

		execute("Pause", true);
		execute("SetMute", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("DesiredMute", "0") }));
		if(uri.compare(0, 16, "x-sonosapi-radio") == 0)
		{
			execute("SetAVTransportURI", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("CurrentURI", uri), SoapValuePair("CurrentURIMetaData", metadata) }));
		}
		else
		{
			execute("RemoveAllTracksFromQueue", true);
			execute("AddURIToQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("EnqueuedURI", uri), SoapValuePair("EnqueuedURIMetaData", metadata), SoapValuePair("DesiredFirstTrackNumberEnqueued", "0"), SoapValuePair("EnqueueAsNext", "0") }));
			execute("SetAVTransportURI", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("CurrentURI", "x-rincon-queue:" + rinconId + "#0"), SoapValuePair("CurrentURIMetaData", "") }));
			execute("Seek", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Unit", "TRACK_NR"), SoapValuePair("Target", std::to_string(1)) }));
		}
		execute("Play", true);

        return std::make_shared<BaseLib::Variable>();
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return Variable::createError(-32500, "Unknown application error.");
}

PVariable SonosPeer::streamLocalInput(PRpcClientInfo clientInfo, bool wait)
{
    try
    {
        std::string rinconId;
        auto channelOneIterator = valuesCentral.find(1);
        if(channelOneIterator != valuesCentral.end())
        {
            auto parameterIterator = channelOneIterator->second.find("ID");
            if(parameterIterator != channelOneIterator->second.end())
            {
                std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
                PVariable variable = _binaryDecoder->decodeResponse(parameterData);
                if(variable) rinconId = variable->stringValue;
            }
        }
        if(rinconId.empty()) return Variable::createError(-1, "Could not get required variables.");

        if(!_isMaster)
        {
            std::shared_ptr<SonosCentral> central(std::dynamic_pointer_cast<SonosCentral>(getCentral()));
            std::unordered_map<int32_t, std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>> peerMap = getPeers();

            for(auto basicPeer : peerMap[1])
            {
                if(!basicPeer->isSender) continue;
                std::shared_ptr<SonosPeer> peer = central->getPeer(basicPeer->id);
                if(!peer) continue;
                peer->setValue(clientInfo, 1, "AV_TRANSPORT_METADATA", std::make_shared<BaseLib::Variable>(std::string("")), wait);
                return peer->setValue(clientInfo, 1, "AV_TRANSPORT_URI", std::make_shared<BaseLib::Variable>(std::string("x-rincon-stream:" + rinconId)), wait);
            }
        }
        else
        {
            setValue(clientInfo, 1, "AV_TRANSPORT_METADATA", std::make_shared<BaseLib::Variable>(std::string("")), wait);
            return setValue(clientInfo, 1, "AV_TRANSPORT_URI", std::make_shared<BaseLib::Variable>(std::string("x-rincon-stream:" + rinconId)), wait);
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
					std::vector<uint8_t> parameterData = valuesCentral[channel][j->second->id].getBinaryData();
					soapValues->push_back(std::pair<std::string, std::string>((*i)->key, _binaryDecoder->decodeResponse(parameterData)->toString()));
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
			BaseLib::Http response;
			try
			{
				_httpClient->sendRequest(soapRequest, response);
				std::string stringResponse(response.getContent().data(), response.getContentSize());
				if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + stringResponse);
				if(response.getHeader().responseCode < 200 || response.getHeader().responseCode > 299)
				{
					//if(response.getHeader().responseCode == -1) serviceMessages->setUnreach(true, false);
					return Variable::createError(-100, "Error sending value to Sonos device: Response code was: " + std::to_string(response.getHeader().responseCode));
				}
				std::shared_ptr<SonosPacket> responsePacket(new SonosPacket(stringResponse));
				packetReceived(responsePacket);
				serviceMessages->setUnreach(false, true);
			}
			catch(const BaseLib::HttpClientException& ex)
			{
				if(ex.responseCode() == -1) serviceMessages->setUnreach(true, false);
				return Variable::createError(-100, "Error sending value to Sonos device: " + std::string(ex.what()));
			}
		}

		auto& rpcConfigurationParameter = valuesCentral[channel][parameter->id];
		std::vector<uint8_t> parameterData = rpcConfigurationParameter.getBinaryData();
		return parameter->convertFromPacket(parameterData, rpcConfigurationParameter.mainRole(), true);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
	return PParameterGroup();
}

bool SonosPeer::getAllValuesHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters)
{
	try
	{
		if(channel == 1)
		{
			if(parameter->id == "IP_ADDRESS")
			{
				std::vector<uint8_t> parameterData;
                auto& rpcConfigurationParameter = valuesCentral[channel][parameter->id];
				parameter->convertToPacket(PVariable(new Variable(_ip)), rpcConfigurationParameter.mainRole(), parameterData);
                rpcConfigurationParameter.setBinaryData(parameterData);
			}
			else if(parameter->id == "PEER_ID")
			{
				std::vector<uint8_t> parameterData;
                auto& rpcConfigurationParameter = valuesCentral[channel][parameter->id];
				parameter->convertToPacket(PVariable(new Variable((int32_t)_peerID)), rpcConfigurationParameter.mainRole(), parameterData);
                rpcConfigurationParameter.setBinaryData(parameterData);
			}
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
    return false;
}

bool SonosPeer::getParamsetHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters)
{
	try
	{
		if(channel == 1)
		{
			if(parameter->id == "IP_ADDRESS")
			{
				std::vector<uint8_t> parameterData;
                auto& rpcConfigurationParameter = valuesCentral[channel][parameter->id];
				parameter->convertToPacket(PVariable(new Variable(_ip)), rpcConfigurationParameter.mainRole(), parameterData);
                rpcConfigurationParameter.setBinaryData(parameterData);
			}
			else if(parameter->id == "PEER_ID")
			{
				std::vector<uint8_t> parameterData;
                auto& rpcConfigurationParameter = valuesCentral[channel][parameter->id];
				parameter->convertToPacket(PVariable(new Variable((int32_t)_peerID)), rpcConfigurationParameter.mainRole(), parameterData);
                rpcConfigurationParameter.setBinaryData(parameterData);
			}
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
    return Variable::createError(-32500, "Unknown application error.");
}

PVariable SonosPeer::putParamset(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, PVariable variables, bool checkAcls, bool onlyPushing)
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

		auto central = getCentral();
		if(!central) return Variable::createError(-32500, "Could not get central.");

		if(type == ParameterGroup::Type::Enum::config)
		{
			bool configChanged = false;
			for(Struct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator channelIterator = configCentral.find(channel);
				if(channelIterator == configCentral.end()) continue;
				std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator parameterIterator = channelIterator->second.find(i->first);
				if(parameterIterator == channelIterator->second.end()) continue;
				BaseLib::Systems::RpcConfigurationParameter& parameter = parameterIterator->second;
				if(!parameter.rpcParameter) continue;
				std::vector<uint8_t> parameterData;
				parameter.rpcParameter->convertToPacket(i->second, parameter.mainRole(), parameterData);
				parameter.setBinaryData(parameterData);
				if(parameter.databaseId > 0) saveParameter(parameter.databaseId, parameterData);
				else saveParameter(0, ParameterGroup::Type::Enum::config, channel, i->first, parameterData);
				GD::out.printInfo("Info: Parameter " + i->first + " of peer " + std::to_string(_peerID) + " and channel " + std::to_string(channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(parameterData) + ".");
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

				if(checkAcls && !clientInfo->acls->checkVariableWriteAccess(central->getPeer(_peerID), channel, i->first)) continue;

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
    return Variable::createError(-32500, "Unknown application error.");
}

PVariable SonosPeer::setValue(BaseLib::PRpcClientInfo clientInfo, uint32_t channel, std::string valueKey, PVariable value, bool wait)
{
	try
	{
		if(!_isMaster && (
				valueKey == "NEXT" ||
				valueKey == "PAUSE" ||
				valueKey == "PLAY" ||
				valueKey == "PLAY_AUDIO_FILE" ||
				valueKey == "PLAY_AUDIO_FILE_UNMUTE" ||
				valueKey == "PLAY_AUDIO_FILE_VOLUME" ||
				valueKey == "PLAY_FADE" ||
				valueKey == "PLAY_FAVORITE" ||
				valueKey == "PLAY_PLAYLIST" ||
				valueKey == "PLAY_RADIO_FAVORITE" ||
				valueKey == "PLAY_TTS" ||
				valueKey == "PLAY_TTS_LANGUAGE" ||
				valueKey == "PLAY_TTS_UNMUTE" ||
				valueKey == "PLAY_TTS_VOICE" ||
                valueKey == "PLAY_TTS_ENGINE" ||
				valueKey == "PLAY_TTS_VOLUME" ||
				valueKey == "PREVIOUS" ||
				valueKey == "STOP" ||
                valueKey == "ADD_SPEAKER" ||
                valueKey == "REMOVE_SPEAKER" ||
                valueKey == "ADD_SPEAKER_BY_SERIAL" ||
                valueKey == "REMOVE_SPEAKER_BY_SERIAL"))
		{
			std::shared_ptr<SonosCentral> central(std::dynamic_pointer_cast<SonosCentral>(getCentral()));
			std::unordered_map<int32_t, std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>> peerMap = getPeers();

			for(auto basicPeer : peerMap[1])
			{
				if(!basicPeer->isSender) continue;
				std::shared_ptr<SonosPeer> peer = central->getPeer(basicPeer->id);
				if(!peer) continue;
				return peer->setValue(clientInfo, channel, valueKey, value, wait);
			}
		}

		Peer::setValue(clientInfo, channel, valueKey, value, wait); //Ignore result, otherwise setHomegerValue might not be executed
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(valueKey.empty()) return Variable::createError(-5, "Value key is empty.");
		if(valuesCentral.find(channel) == valuesCentral.end()) return Variable::createError(-2, "Unknown channel.");
		if(setHomegearValue(channel, valueKey, value)) return std::make_shared<Variable>(VariableType::tVoid);
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return Variable::createError(-5, "Unknown parameter.");
		auto& parameter = valuesCentral[channel][valueKey];
		PParameter rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
		if(!rpcParameter) return Variable::createError(-5, "Unknown parameter.");
		if(rpcParameter->service)
		{
			if(channel == 0 && value->type == VariableType::tBoolean)
			{
				if(serviceMessages->set(valueKey, value->booleanValue)) return std::make_shared<Variable>(VariableType::tVoid);
			}
			else if(value->type == VariableType::tInteger) serviceMessages->set(valueKey, value->integerValue, channel);
		}
		if(rpcParameter->logical->type == ILogical::Type::tAction && !value->booleanValue) value->booleanValue = true;
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>());
		std::shared_ptr<std::vector<PVariable>> values(new std::vector<PVariable>());

		std::vector<uint8_t> parameterData;
		rpcParameter->convertToPacket(value, parameter.mainRole(), parameterData);
		value = rpcParameter->convertFromPacket(parameterData, parameter.mainRole(), true);

		valueKeys->push_back(valueKey);
		values->push_back(value);

		if(rpcParameter->physical->operationType == IPhysical::OperationType::Enum::command)
		{
			if(valueKey == "PLAY_FAVORITE")
			{
				BaseLib::PVariable result = playBrowsableContent(value->stringValue, "FV:2", "FAVORITES");
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
				std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>& peers = peerMap[1];

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
            else if(valueKey == "STREAM_LOCAL_INPUT")
            {
                return streamLocalInput(clientInfo, wait);
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
						for(std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator j = valuesCentral[channel].begin(); j != valuesCentral[channel].end(); ++j)
						{
							if(!j->second.rpcParameter) continue;
							if((*i)->parameterId == j->second.rpcParameter->physical->groupId)
							{
								if((*i)->key.empty()) continue;
								std::vector<uint8_t> parameterData2 = j->second.getBinaryData();
								soapValues->push_back(std::pair<std::string, std::string>((*i)->key, _binaryDecoder->decodeResponse(parameterData2)->toString()));
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
					BaseLib::Http response;
					try
					{
						_httpClient->sendRequest(soapRequest, response);
						std::string stringResponse(response.getContent().data(), response.getContentSize());
						if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + stringResponse);
						if(response.getHeader().responseCode < 200 || response.getHeader().responseCode > 299)
						{
							GD::out.printWarning("Warning: Error in UPnP request: Response code was: " + std::to_string(response.getHeader().responseCode));
							GD::out.printMessage("Request was: \n" + soapRequest);
							return Variable::createError(-100, "Error sending value to Sonos device: Response code was: " + std::to_string(response.getHeader().responseCode));
						}
					}
					catch(const BaseLib::HttpException& ex)
					{
						GD::out.printWarning("Warning: Error in UPnP request: " + std::string(ex.what()));
						GD::out.printMessage("Request was: \n" + soapRequest);
						return Variable::createError(-100, "Error sending value to Sonos device: " + std::string(ex.what()));
					}
					catch(const BaseLib::HttpClientException& ex)
					{
						GD::out.printWarning("Warning: Error in UPnP request: " + std::string(ex.what()));
						GD::out.printMessage("Request was: \n" + soapRequest);
						return Variable::createError(-100, "Error sending value to Sonos device: " + std::string(ex.what()));
					}
					catch(const std::exception& ex)
					{
						GD::out.printWarning("Warning: Error in UPnP request: " + std::string(ex.what()));
						GD::out.printMessage("Request was: \n" + soapRequest);
						return Variable::createError(-100, "Error sending value to Sonos device: " + std::string(ex.what()));
					}
				}
			}
		}
		else if(rpcParameter->physical->operationType != IPhysical::OperationType::Enum::store) return Variable::createError(-6, "Only interface types \"store\" and \"command\" are supported for this device family.");

		parameter.setBinaryData(parameterData);
		if(parameter.databaseId > 0) saveParameter(parameter.databaseId, parameterData);
		else saveParameter(0, ParameterGroup::Type::Enum::variables, channel, valueKey, parameterData);

        std::string address(_serialNumber + ":" + std::to_string(channel));
        raiseEvent(clientInfo->initInterfaceId, _peerID, channel, valueKeys, values);
        raiseRPCEvent(clientInfo->initInterfaceId, _peerID, channel, address, valueKeys, values);

		return std::make_shared<BaseLib::Variable>();
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
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
}

void SonosPeer::playLocalFile(std::string filename, bool now, bool unmute, int32_t volume)
{
	try
	{
		std::unique_lock<std::timed_mutex> playLocalFileGuard(_playLocalFileMutex, std::chrono::milliseconds(1000));
		if(!playLocalFileGuard)
		{
			GD::out.printWarning("Warning: Not playing file " + filename + ", because a file is already being played back.");
			return;
		}
		if(serviceMessages->getUnreach())
		{
			GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
			return;
		}
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
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}
			execute("GetVolume");
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}
			execute("GetMute");
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}
			execute("GetTransportInfo");
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}
		}

		std::string playlistFilename = filename.substr(0, filename.size() - 4) + ".m3u";
		BaseLib::HelperFunctions::stringReplace(playlistFilename, "/", "_");
		std::string playlistContent = "#EXTM3U\n#EXTINF:0,<Homegear><TTS><TTS>\nhttp://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + '/' + filename + '\n';
		std::string playlistFilepath = tempPath + playlistFilename;
		BaseLib::Io::writeFile(playlistFilepath, playlistContent);
		playlistFilename = BaseLib::Http::encodeURL(playlistFilename);

		std::string silence2sPlaylistFilename = "silence_2s.m3u";
		playlistContent = "#EXTM3U\n#EXTINF:0,<Homegear><TTS><TTS>\nhttp://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + "/Silence_1s.mp3\n";
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
		std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>& peers = peerMap[1];

		auto channelOneIterator = valuesCentral.find(1);
		if(channelOneIterator == valuesCentral.end())
		{
			GD::out.printError("Error: Channel 1 not found.");
			return;
		}
		if(now)
		{
			execute("GetMediaInfo");
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}

			std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("AV_TRANSPORT_URI");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) currentTransportUri = variable->stringValue;
				if(currentTransportUri.compare(0, 14, "x-rincon-queue") != 0)
				{
					GD::out.printInfo("Info (peer " + std::to_string(_peerID) + "): Currently playing non-rincon queue (e. g. radio).");
					setQueue = true;

					parameterIterator = channelOneIterator->second.find("AV_TRANSPORT_METADATA");
					if(parameterIterator != channelOneIterator->second.end())
					{
						parameterData = parameterIterator->second.getBinaryData();
						PVariable variable = _binaryDecoder->decodeResponse(parameterData);
						if(variable) currentTransportUriMetadata = variable->stringValue;
					}
				}
			}

			parameterIterator = channelOneIterator->second.find("VOLUME");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) volumeState = variable->integerValue;
			}

			parameterIterator = channelOneIterator->second.find("MUTE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) muteState = (variable->stringValue == "1");
			}

			parameterIterator = channelOneIterator->second.find("CURRENT_TRACK");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) trackNumberState = variable->integerValue;
			}

			parameterIterator = channelOneIterator->second.find("CURRENT_TRACK_RELATIVE_TIME");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) seekTimeState = variable->stringValue;
			}

			parameterIterator = channelOneIterator->second.find("TRANSPORT_STATE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) transportState = variable->stringValue;
			}

			parameterIterator = channelOneIterator->second.find("CURRENT_PLAY_MODE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) playmodeState = variable->stringValue;
			}

			parameterIterator = channelOneIterator->second.find("ID");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) rinconId = variable->stringValue;
			}

			execute("Pause", true);
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}

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
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}
		}

		std::string playlistUri = "http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + '/' + silence10sPlaylistFilename;
		bool result = execute("AddURIToQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("EnqueuedURI", playlistUri), SoapValuePair("EnqueuedURIMetaData", ""), SoapValuePair("DesiredFirstTrackNumberEnqueued", "1"), SoapValuePair("EnqueueAsNext", "1") }), true);
		if(!result)
		{
			GD::out.printWarning("Warning: Can't play file " + filename + ", because the speaker is not master.");
			return;
		}
		if(serviceMessages->getUnreach())
		{
			GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
			return;
		}

		playlistUri = "http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + '/' + playlistFilename;
		execute("AddURIToQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("EnqueuedURI", playlistUri), SoapValuePair("EnqueuedURIMetaData", ""), SoapValuePair("DesiredFirstTrackNumberEnqueued", "1"), SoapValuePair("EnqueueAsNext", "1") }));
		if(serviceMessages->getUnreach())
		{
			GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
			return;
		}

		playlistUri = "http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + '/' + silence2sPlaylistFilename;
		execute("AddURIToQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("EnqueuedURI", playlistUri), SoapValuePair("EnqueuedURIMetaData", ""), SoapValuePair("DesiredFirstTrackNumberEnqueued", "1"), SoapValuePair("EnqueueAsNext", "1") }));
		if(serviceMessages->getUnreach())
		{
			GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
			return;
		}

		if(now)
		{
			execute("SetAVTransportURI", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("CurrentURI", "x-rincon-queue:" + rinconId + "#0"), SoapValuePair("CurrentURIMetaData", "") }));
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}

			//std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
			//std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("FIRST_TRACK_NUMBER_ENQUEUED");
			/*if(parameterIterator != channelOneIterator->second.end())
			{
				PVariable variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
				if(variable) trackNumber = variable->integerValue;
			}*/
			_currentTrack = 1;

			execute("Seek", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Unit", "TRACK_NR"), SoapValuePair("Target", std::to_string(1)) }));
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}
			execute("Play");
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(2000));

			while(!serviceMessages->getUnreach() && (_currentTrack == 1 || _currentTrack == 2))
			{
				for(int32_t i = 0; i < 50; i++)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					if(_currentTrack != 1 && _currentTrack != 2) break;
				}

				execute("GetPositionInfo");

				std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("CURRENT_TRACK");
				if(parameterIterator != channelOneIterator->second.end())
				{
					std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
					PVariable variable = _binaryDecoder->decodeResponse(parameterData);
					if(!variable || (variable->integerValue != 1 && variable->integerValue != 2)) break;
				}
				else break;

				execute("GetTransportInfo");

				parameterIterator = channelOneIterator->second.find("TRANSPORT_STATE");
				if(parameterIterator != channelOneIterator->second.end())
				{
					std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
					PVariable variable = _binaryDecoder->decodeResponse(parameterData);
					if(!variable || (variable->stringValue != "PLAYING" && variable->stringValue != "TRANSITIONING")) break;
				}
				else break;
			}

			//Pause often causes errors at this point
			execute("SetVolume", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("DesiredVolume", "0") }));
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}
			for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = peers.begin(); i != peers.end(); ++i)
			{
				std::shared_ptr<SonosPeer> peer = central->getPeer((*i)->id);
				if(!peer) continue;
				peer->setVolume(0);
			}
			if(muteState) execute("SetMute", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("DesiredMute", std::to_string((int32_t)muteState)) }));
			execute("RemoveTrackFromQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("ObjectID", "Q:0/" + std::to_string(1)) }));
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}
			execute("RemoveTrackFromQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("ObjectID", "Q:0/" + std::to_string(1)) }));
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}
			execute("RemoveTrackFromQueue", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("ObjectID", "Q:0/" + std::to_string(1)) }));
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}
			if(trackNumberState > 0) execute("Seek", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Unit", "TRACK_NR"), SoapValuePair("Target", std::to_string(trackNumberState)) }), true);
			if(!seekTimeState.empty()) execute("Seek", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Unit", "REL_TIME"), SoapValuePair("Target", seekTimeState) }), true);

			if(setQueue) execute("SetAVTransportURI", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("CurrentURI", currentTransportUri), SoapValuePair("CurrentURIMetaData", currentTransportUriMetadata) }));
			if(serviceMessages->getUnreach())
			{
				GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
				return;
			}

			if(transportState == "PLAYING")
			{
				GD::out.printInfo("Info (peer " + std::to_string(_peerID) + "): Resuming playback, because TRANSPORT_STATE was PLAYING.");
				if(setQueue) execute("Play", true);
				if(serviceMessages->getUnreach())
				{
					GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
					return;
				}

				for(int32_t i = 0; i < 10; i++)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));

					std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("TRANSPORT_STATE");
					if(parameterIterator != channelOneIterator->second.end())
					{
						std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
						PVariable variable = _binaryDecoder->decodeResponse(parameterData);
						if(!variable || (variable->stringValue != "TRANSITIONING")) break;
					}
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				execute("RampToVolume", PSoapValues(new SoapValues{ SoapValuePair("InstanceID", "0"), SoapValuePair("Channel", "Master"), SoapValuePair("RampType", "AUTOPLAY_RAMP_TYPE"), SoapValuePair("DesiredVolume", std::to_string(volumeState)), SoapValuePair("ResetVolumeAfter", "false"), SoapValuePair("ProgramURI", "") }), true);
				if(serviceMessages->getUnreach())
				{
					GD::out.printWarning("Warning: Not playing file " + filename + ", because a speaker is unreachable.");
					return;
				}

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
				execute("Pause", true);
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
			std::string voice;
			std::string engine;

			std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
			if(channelOneIterator == valuesCentral.end())
			{
				GD::out.printError("Error: Channel 1 not found.");
				return true;
			}

			std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("PLAY_TTS_UNMUTE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) unmute = variable->booleanValue;
			}

			parameterIterator = channelOneIterator->second.find("PLAY_TTS_VOLUME");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) volume = variable->integerValue;
			}

			parameterIterator = channelOneIterator->second.find("PLAY_TTS_LANGUAGE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) language = variable->stringValue;
				if(!BaseLib::HelperFunctions::isAlphaNumeric(language, std::unordered_set<char>{'-', '_'}))
				{
					GD::out.printError("Error: Language is not alphanumeric.");
					language = "en-US";
				}
			}

			parameterIterator = channelOneIterator->second.find("PLAY_TTS_VOICE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) voice = variable->stringValue;
				if(!BaseLib::HelperFunctions::isAlphaNumeric(language, std::unordered_set<char>{'-', '_'}))
				{
					GD::out.printError("Error: Voice is not alphanumeric.");
					voice = "Justin";
				}
			}

          parameterIterator = channelOneIterator->second.find("PLAY_TTS_ENGINE");
          if(parameterIterator != channelOneIterator->second.end())
          {
            std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
            PVariable variable = _binaryDecoder->decodeResponse(parameterData);
            if(variable) engine = variable->stringValue;
            if(!BaseLib::HelperFunctions::isAlphaNumeric(language, std::unordered_set<char>{'-', '_'}))
            {
              GD::out.printError("Error: Engine is not alphanumeric.");
              engine = "standard";
            }
          }

			std::string audioPath = GD::bl->settings.tempPath() + "sonos/";
			std::string filename;
			BaseLib::HelperFunctions::stringReplace(value->stringValue, "\"", "");
			std::string execPath = ttsProgram + ' ' + language + ' ' + voice + " \"" + value->stringValue + "\"" + (!engine.empty() ? " " + engine : "");
            auto exitCode = BaseLib::ProcessManager::exec(execPath, _bl->fileDescriptorManager.getMax(), filename);
			if(exitCode != 0)
			{
				GD::out.printError("Error: Error executing program to generate TTS audio file (exit code " + std::to_string(exitCode) + "): \"" + ttsProgram + ' ' + language + ' ' + value->stringValue + "\"");
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

			std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
			if(channelOneIterator == valuesCentral.end())
			{
				GD::out.printError("Error: Channel 1 not found.");
				return true;
			}

			std::unordered_map<std::string, BaseLib::Systems::RpcConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("PLAY_AUDIO_FILE_UNMUTE");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
				if(variable) unmute = variable->booleanValue;
			}

			parameterIterator = channelOneIterator->second.find("PLAY_AUDIO_FILE_VOLUME");
			if(parameterIterator != channelOneIterator->second.end())
			{
				std::vector<uint8_t> parameterData = parameterIterator->second.getBinaryData();
				PVariable variable = _binaryDecoder->decodeResponse(parameterData);
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
	return false;
}

}
