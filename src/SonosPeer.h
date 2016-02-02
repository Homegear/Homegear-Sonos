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

#ifndef SONOSPEER_H_
#define SONOSPEER_H_

#include "homegear-base/BaseLib.h"

#include <list>

using namespace BaseLib;
using namespace BaseLib::DeviceDescription;

namespace Sonos
{
class SonosCentral;
class SonosPacket;

class FrameValue
{
public:
	std::list<uint32_t> channels;
	std::vector<uint8_t> value;
};

class FrameValues
{
public:
	std::string frameID;
	std::list<uint32_t> paramsetChannels;
	ParameterGroup::Type::Enum parameterSetType;
	std::unordered_map<std::string, FrameValue> values;
};

class SonosPeer : public BaseLib::Systems::Peer
{
public:
	SonosPeer(uint32_t parentID, IPeerEventSink* eventHandler);
	SonosPeer(int32_t id, std::string serialNumber, uint32_t parentID, IPeerEventSink* eventHandler);
	virtual ~SonosPeer();
	void init();

	//Features
	virtual bool wireless() { return false; }
	//End features

	// {{{ In table variables
	virtual void setIp(std::string value);
	// }}}

	virtual void setRinconId(std::string value);

	void worker();
	virtual std::string handleCliCommand(std::string command);

	virtual bool load(BaseLib::Systems::ICentral* central);
    virtual void savePeers() {}

	virtual int32_t getChannelGroupedWith(int32_t channel) { return -1; }
	virtual int32_t getNewFirmwareVersion() { return 0; }
	virtual std::string getFirmwareVersionString() { return Peer::getFirmwareVersionString(); }
	virtual std::string getFirmwareVersionString(int32_t firmwareVersion) { return _firmwareVersionString; }
    virtual bool firmwareUpdateAvailable() { return false; }

    void packetReceived(std::shared_ptr<SonosPacket> packet);

    std::string printConfig();

    /**
	 * {@inheritDoc}
	 */
    virtual void homegearShuttingDown();

	//RPC methods
	virtual PVariable getDeviceInfo(BaseLib::PRpcClientInfo clientInfo, std::map<std::string, bool> fields);
	virtual PVariable getParamsetDescription(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual PVariable getParamset(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel);
	virtual PVariable getValue(BaseLib::PRpcClientInfo clientInfo, uint32_t channel, std::string valueKey, bool requestFromDevice, bool asynchronous);
	virtual PVariable putParamset(BaseLib::PRpcClientInfo clientInfo, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, PVariable variables, bool onlyPushing = false);
	virtual PVariable setValue(BaseLib::PRpcClientInfo clientInfo, uint32_t channel, std::string valueKey, PVariable value, bool wait);
	//End RPC methods
protected:
	class UpnpFunctionEntry
	{
	public:
		std::string& service() { return _service; }
		std::string& path() { return _path; }
		std::shared_ptr<std::vector<std::pair<std::string, std::string>>> soapValues() { return _soapValues; }

		UpnpFunctionEntry(std::string service, std::string path, std::shared_ptr<std::vector<std::pair<std::string, std::string>>> soapValues)
		{
			_service = service;
			_path = path;
			_soapValues = soapValues;
		}
	private:
		std::string _service;
		std::string _path;
		std::shared_ptr<std::vector<std::pair<std::string, std::string>>> _soapValues;
	};

	bool _shuttingDown = false;
	bool _getOneMorePositionInfo = true;
	std::shared_ptr<BaseLib::RPC::RPCEncoder> _binaryEncoder;
	std::shared_ptr<BaseLib::RPC::RPCDecoder> _binaryDecoder;
	std::shared_ptr<BaseLib::HTTPClient> _httpClient;
	int32_t _lastAvTransportSubscription = 0;
	int32_t _lastPositionInfo = 0;
	int32_t _lastAvTransportInfo = 0;

	typedef std::map<std::string, UpnpFunctionEntry> UpnpFunctions;
	typedef std::pair<std::string, UpnpFunctionEntry> UpnpFunctionPair;
	typedef std::vector<std::pair<std::string, std::string>> SoapValues;
	typedef std::shared_ptr<std::vector<std::pair<std::string, std::string>>> PSoapValues;
	typedef std::pair<std::string, std::string> SoapValuePair;
	UpnpFunctions _upnpFunctions;

	virtual void loadVariables(BaseLib::Systems::ICentral* central, std::shared_ptr<BaseLib::Database::DataTable>& rows);
    virtual void saveVariables();

	virtual std::shared_ptr<BaseLib::Systems::ICentral> getCentral();
	void getValuesFromPacket(std::shared_ptr<SonosPacket> packet, std::vector<FrameValues>& frameValue);
	bool setHomegearValue(uint32_t channel, std::string valueKey, PVariable value);

	/**
	 * {@inheritDoc}
	 */
	virtual PVariable getValueFromDevice(PParameter& parameter, int32_t channel, bool asynchronous);

	virtual PParameterGroup getParameterSet(int32_t channel, ParameterGroup::Type::Enum type);

	void execute(std::string& functionName, std::string& service, std::string& path, PSoapValues& soapValues, bool ignoreErrors = false);

	void execute(std::string functionName, bool ignoreErrors = false);

	void execute(std::string functionName, PSoapValues soapValues, bool ignoreErrors = false);

	void sendSoapRequest(std::string& request, bool ignoreErrors = false);

	void playLocalFile(std::string filename, bool now, bool unmute, int32_t volume);

	PVariable playBrowsableContent(std::string& title, std::string browseId, std::string listVariable);

	// {{{ Hooks
		/**
		 * {@inheritDoc}
		 */
		virtual bool getAllValuesHook2(PRpcClientInfo clientInfo, PParameter parameter, uint32_t channel, PVariable parameters);
	// }}}
};

}

#endif
