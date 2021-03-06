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

#include "SonosPacket.h"
#include "GD.h"

namespace Sonos
{
SonosPacket::SonosPacket()
{
	_values.reset(new std::unordered_map<std::string, std::string>());
	_valuesToSet.reset(new std::vector<std::pair<std::string, std::string>>());
}

SonosPacket::SonosPacket(std::string& soap, int64_t timeReceived)
{
	try
	{
		BaseLib::HelperFunctions::trim(soap);
		_values.reset(new std::unordered_map<std::string, std::string>());
		_valuesToSet.reset(new std::vector<std::pair<std::string, std::string>>());
		_timeReceived = timeReceived;
		if(soap.empty()) return;
		xml_document doc;
		doc.parse<parse_no_entity_translation>(&soap.at(0));
		xml_node* node = doc.first_node("s:Envelope");
		if(!node) //Info packet
		{
			xml_node* node = doc.first_node("Event");
			if(!node)
			{
				GD::out.printWarning("Warning: Tried to parse invalid packet.");
				return;
			}
			_functionName = "InfoBroadcast";
			node = node->first_node("InstanceID");
			if(!node)
			{
				GD::out.printWarning("Warning: Tried to parse invalid packet.");
				return;
			}
			for(xml_node* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
			{
				std::string name(subNode->name());
				xml_attribute* attr = subNode->first_attribute("val");
				xml_attribute* channel = subNode->first_attribute("channel");
				if(channel) name.append(std::string(channel->value()));
				if(!attr)
				{
					GD::out.printWarning("Warning: Tried to parse element without attribute: " + name);
					continue;
				}
				std::string value(attr->value());
				if(name == "CurrentTrackMetaData" || name == "TrackMetaData")
				{
					_values->operator [](name) = value;
					_currentTrackMetadata.reset(new std::unordered_map<std::string, std::string>());
					if(value.empty()) continue;
					std::string xml;
					BaseLib::Html::unescapeHtmlEntities(value, xml);
					xml_document metadataDoc;
					metadataDoc.parse<parse_no_entity_translation>((char*)xml.data());
					xml_node* metadataNode = metadataDoc.first_node("DIDL-Lite");
					if(!metadataNode) continue;
					metadataNode = metadataNode->first_node("item");
					if(!metadataNode) continue;
					for(xml_attribute* metadataAttribute = metadataNode->first_attribute(); metadataAttribute; metadataAttribute = metadataAttribute->next_attribute())
					{
						_currentTrackMetadata->operator [](std::string(metadataAttribute->name())) = std::string(metadataAttribute->value());
					}
					for(xml_node* metadataSubNode = metadataNode->first_node(); metadataSubNode; metadataSubNode = metadataSubNode->next_sibling())
					{
						std::string metadataName(metadataSubNode->name());
						if(metadataName == "res")
						{
							_currentTrackMetadata->operator [](metadataName) = std::string(metadataSubNode->value());
							for(xml_attribute* metadataAttribute = metadataSubNode->first_attribute(); metadataAttribute; metadataAttribute = metadataAttribute->next_attribute())
							{
								_currentTrackMetadata->operator [](std::string(metadataAttribute->name())) = std::string(metadataAttribute->value());
							}
						}
						else
						{
							_currentTrackMetadata->operator [](std::string(metadataSubNode->name())) = std::string(metadataSubNode->value());
						}
					}
				}
				else if(name == "r:NextTrackMetaData")
				{
					_values->operator [](name) = value;
					_nextTrackMetadata.reset(new std::unordered_map<std::string, std::string>());
					if(value.empty()) continue;
					std::string xml;
					BaseLib::Html::unescapeHtmlEntities(value, xml);
					xml_document metadataDoc;
					metadataDoc.parse<parse_no_entity_translation>((char*)xml.data());
					xml_node* metadataNode = metadataDoc.first_node("DIDL-Lite");
					if(!metadataNode) continue;
					metadataNode = metadataNode->first_node("item");
					if(!metadataNode) continue;
					for(xml_attribute* metadataAttribute = metadataNode->first_attribute(); metadataAttribute; metadataAttribute = metadataAttribute->next_attribute())
					{
						_nextTrackMetadata->operator [](std::string(metadataAttribute->name())) = std::string(metadataAttribute->value());
					}
					for(xml_node* metadataSubNode = metadataNode->first_node(); metadataSubNode; metadataSubNode = metadataSubNode->next_sibling())
					{
						std::string metadataName(metadataSubNode->name());
						if(metadataName == "res")
						{
							_nextTrackMetadata->operator [](metadataName) = std::string(metadataSubNode->value());
							for(xml_attribute* metadataAttribute = metadataSubNode->first_attribute(); metadataAttribute; metadataAttribute = metadataAttribute->next_attribute())
							{
								_nextTrackMetadata->operator [](std::string(metadataAttribute->name())) = std::string(metadataAttribute->value());
							}
						}
						else
						{
							_nextTrackMetadata->operator [](std::string(metadataSubNode->name())) = std::string(metadataSubNode->value());
						}
					}
				}
				else if(name == "AVTransportURIMetaData")
				{
					_values->operator [](name) = value;
					_avTransportUriMetaData.reset(new std::unordered_map<std::string, std::string>());
					if(value.empty()) continue;
					std::string xml;
					BaseLib::Html::unescapeHtmlEntities(value, xml);
					xml_document metadataDoc;
					metadataDoc.parse<parse_no_entity_translation>((char*)xml.data());
					xml_node* metadataNode = metadataDoc.first_node("DIDL-Lite");
					if(!metadataNode) continue;
					metadataNode = metadataNode->first_node("item");
					if(!metadataNode) continue;
					for(xml_attribute* metadataAttribute = metadataNode->first_attribute(); metadataAttribute; metadataAttribute = metadataAttribute->next_attribute())
					{
						_avTransportUriMetaData->operator [](std::string(metadataAttribute->name())) = std::string(metadataAttribute->value());
					}
					for(xml_node* metadataSubNode = metadataNode->first_node(); metadataSubNode; metadataSubNode = metadataSubNode->next_sibling())
					{
						std::string metadataName(metadataSubNode->name());
						_avTransportUriMetaData->operator [](std::string(metadataSubNode->name())) = std::string(metadataSubNode->value());
					}
				}
				else if(name == "NextAVTransportURIMetaData")
				{
					_values->operator [](name) = value;
					_nextAvTransportUriMetaData.reset(new std::unordered_map<std::string, std::string>());
					if(value.empty()) continue;
					std::string xml;
					BaseLib::Html::unescapeHtmlEntities(value, xml);
					xml_document metadataDoc;
					metadataDoc.parse<parse_no_entity_translation>((char*)xml.data());
					xml_node* metadataNode = metadataDoc.first_node("DIDL-Lite");
					if(!metadataNode) continue;
					metadataNode = metadataNode->first_node("item");
					if(!metadataNode) continue;
					for(xml_attribute* metadataAttribute = metadataNode->first_attribute(); metadataAttribute; metadataAttribute = metadataAttribute->next_attribute())
					{
						_nextAvTransportUriMetaData->operator [](std::string(metadataAttribute->name())) = std::string(metadataAttribute->value());
					}
					for(xml_node* metadataSubNode = metadataNode->first_node(); metadataSubNode; metadataSubNode = metadataSubNode->next_sibling())
					{
						std::string metadataName(metadataSubNode->name());
						_nextAvTransportUriMetaData->operator [](std::string(metadataSubNode->name())) = std::string(metadataSubNode->value());
					}
				}
				else if(name == "r:EnqueuedTransportURIMetaData")
				{
					_values->operator [](name) = value;
					_enqueuedTransportUriMetaData.reset(new std::unordered_map<std::string, std::string>());
					if(value.empty()) continue;
					std::string xml;
					BaseLib::Html::unescapeHtmlEntities(value, xml);
					xml_document metadataDoc;
					metadataDoc.parse<parse_no_entity_translation>((char*)xml.data());
					xml_node* metadataNode = metadataDoc.first_node("DIDL-Lite");
					if(!metadataNode) continue;
					metadataNode = metadataNode->first_node("item");
					if(!metadataNode) continue;
					for(xml_attribute* metadataAttribute = metadataNode->first_attribute(); metadataAttribute; metadataAttribute = metadataAttribute->next_attribute())
					{
						_enqueuedTransportUriMetaData->operator [](std::string(metadataAttribute->name())) = std::string(metadataAttribute->value());
					}
					for(xml_node* metadataSubNode = metadataNode->first_node(); metadataSubNode; metadataSubNode = metadataSubNode->next_sibling())
					{
						std::string metadataName(metadataSubNode->name());
						_enqueuedTransportUriMetaData->operator [](std::string(metadataSubNode->name())) = std::string(metadataSubNode->value());
					}
				}
				else
				{
					_values->operator [](name) = value;
				}
			}
		}
		else
		{
			node = node->first_node("s:Body");
			if(!node) return;
			node = node->first_node();
			if(!node) return;
			_functionName = std::string(node->name());
			if(_functionName.size() > 2) _functionName = _functionName.substr(2);
			if(_functionName == "BrowseResponse")
			{
				xml_node* subNode = node->first_node("Result");
				if(!subNode) return;
				std::string value(subNode->value());
				std::string xml;
				BaseLib::Html::unescapeHtmlEntities(value, xml);
				xml_document metadataDoc;
				metadataDoc.parse<parse_no_entity_translation>((char*)xml.data());
				xml_node* metadataNode = metadataDoc.first_node("DIDL-Lite");
				if(!metadataNode) return;
				_browseResult.reset(new std::pair<std::string, BaseLib::PVariable>("", BaseLib::PVariable(new Variable(VariableType::tArray))));
				for(xml_node* metadataSubNode = metadataNode->first_node(); metadataSubNode; metadataSubNode = metadataSubNode->next_sibling())
				{
					std::string nodeName(metadataSubNode->name());
					if(nodeName != "item" && nodeName != "container") continue;
					xml_attribute* parentIdAttr = metadataSubNode->first_attribute("parentID");
					xml_node* titleNode = metadataSubNode->first_node("dc:title");
					xml_node* albumArtNode = metadataSubNode->first_node("upnp:albumArtURI");
					xml_node* uriNode = metadataSubNode->first_node("res");
					xml_node* metadataNode = metadataSubNode->first_node("r:resMD");
					xml_node* classNode = metadataSubNode->first_node("upnp:class");
					xml_node* artistNode = metadataSubNode->first_node("dc:creator");
					xml_node* albumNode = metadataSubNode->first_node("upnp:album");
					if(!parentIdAttr || !titleNode || !uriNode || !classNode) continue;

					if(_browseResult->first.empty())
					{
						std::string parentId(parentIdAttr->value());
						_browseResult->first = parentId;
					}

					PVariable item(new Variable(VariableType::tStruct));
					_browseResult->second->arrayValue->push_back(item);

					std::string metadata;
					if(metadataNode)
					{
						metadata = std::string(metadataNode->value());
						int32_t pos1 = (signed)metadata.find("&lt;upnp:class&gt;");
						int32_t pos2 = (signed)metadata.find("&lt;/upnp:class&gt;");
						if(pos1 < 0 || pos2 < 0) continue;
						std::string upnpClass(classNode->value());
						metadata.replace(pos1 + 18, pos2 - pos1 - 18, upnpClass);
					}

					item->structValue->insert(StructElement("TITLE", PVariable(new Variable(std::string(titleNode->value())))));
					if(albumNode) item->structValue->insert(StructElement("ALBUM", PVariable(new Variable(std::string(albumNode->value())))));
					if(artistNode) item->structValue->insert(StructElement("ARTIST", PVariable(new Variable(std::string(artistNode->value())))));
					if(albumArtNode) item->structValue->insert(StructElement("ALBUMART", PVariable(new Variable(std::string(albumArtNode->value())))));
					item->structValue->insert(StructElement("AV_TRANSPORT_URI", PVariable(new Variable(std::string(uriNode->value())))));
					item->structValue->insert(StructElement("AV_TRANSPORT_URI_METADATA", PVariable(new Variable(metadata))));
				}
			}
			else
			{
				for(xml_node* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
				{
					_values->operator [](std::string(subNode->name())) = std::string(subNode->value());
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, std::string(ex.what()) + " Packet was: " + soap);
    }
}

SonosPacket::SonosPacket(std::string& soap, std::string serialNumber, int64_t timeReceived) : SonosPacket(soap, timeReceived)
{
	_serialNumber = serialNumber;
}

SonosPacket::SonosPacket(xml_node* node, int64_t timeReceived )
{
	try
	{
		if(!node) return;
		_values.reset(new std::unordered_map<std::string, std::string>());
		_valuesToSet.reset(new std::vector<std::pair<std::string, std::string>>());
		_timeReceived = timeReceived;
		_functionName = "InfoBroadcast2";
		for(xml_node* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
		{
			_values->operator [](std::string(subNode->name())) = std::string(subNode->value());
		}
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

SonosPacket::SonosPacket(xml_node* node, std::string serialNumber, int64_t timeReceived) : SonosPacket(node, timeReceived)
{
	_serialNumber = serialNumber;
}

SonosPacket::SonosPacket(std::string& ip, std::string& path, std::string& soapAction, std::string& schema, std::string& functionName, std::shared_ptr<std::vector<std::pair<std::string, std::string>>> valuesToSet)
{
	_ip = ip;
	_path = path;
	_soapAction = soapAction;
	_schema = schema;
	_functionName = functionName;
	_valuesToSet = valuesToSet;
	if(!_valuesToSet) _valuesToSet.reset(new std::vector<std::pair<std::string, std::string>>());
	_values.reset(new std::unordered_map<std::string, std::string>());
}

SonosPacket::~SonosPacket()
{
}

void SonosPacket::getSoapRequest(std::string& request)
{
	try
	{
		request = "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body><u:" + _functionName + " xmlns:u=\"" + _schema + "\">";
		for(std::vector<std::pair<std::string, std::string>>::iterator i = _valuesToSet->begin(); i != _valuesToSet->end(); ++i)
		{
			request += '<' + i->first + '>' + i->second + "</" + i->first + '>';
		}
		request += "</u:" + _functionName + "></s:Body></s:Envelope>";

		std::string header = "POST " + _path + " HTTP/1.1\r\nCONNECTION: close\r\nHOST: " + _ip + ":1400\r\nCONTENT-LENGTH: " + std::to_string(request.size()) + "\r\nCONTENT-TYPE: text/xml; charset=\"utf-8\"\r\nSOAPACTION: \"" + _soapAction + "\"\r\n\r\n";
		request.insert(request.begin(), header.begin(), header.end());
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

}
