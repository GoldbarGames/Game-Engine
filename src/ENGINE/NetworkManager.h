#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H
#pragma once

#pragma warning(disable: 4005)
#pragma warning(disable: 4996)

#define USE_NETWORKING 1
#define _SILENCE_ALL_CXX20_DEPRECATION_WARNINGS 1
#define _SILENCE_CXX20_CISO646_REMOVED_WARNING 1

#include <SDL2/SDL.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

#ifndef EMSCRIPTEN
#include <SDL2/SDL_net.h>
#endif


#include "Entity.h"

#ifndef EMSCRIPTEN

enum class Protocol { None, TCP_Client, TCP_Server, UDP_Client, UDP_Server };

class KINJO_API NetworkManager
{
public:
	Protocol protocol = Protocol::None;
	const char* host;
	const char* port;

	std::string messageToSend = "";
	std::string messageReceived = "";

	int quit, len;
	char buffer[512];

	TCPsocket tcpSocket;
	TCPsocket clientSocket;
	IPaddress ipAddress;
	IPaddress* remoteIP;

	UDPsocket udpSocket;
	UDPpacket* udpPacket;
	IPaddress serverAddress;

	virtual void Init(Protocol newProtocol);
	virtual void Update();

	virtual void TCPClient();
	virtual void TCPServer();
	virtual void UDPClient();
	virtual void UDPServer();

	NetworkManager(const char* h, const char* p);
	~NetworkManager();

	virtual void ReadMessage(Game& game);

	virtual std::string ConvertToData(const Entity& entity);
	virtual Entity ConvertFromData(const char* data, int len);

	void AddToMessage(const std::string& entity, int id, const std::string& key, int value, bool condition);
	std::unordered_map<std::string, int> mapKeysToIndices;
	std::unordered_map<std::string, int> mapEntitiesToIndices;

	std::vector<std::string> mapIndicesToKeys;
	std::vector<std::string> mapIndicesToEntities;

	std::string curlPostRequest(const char* url, const char* data, const char* userpwd);
	std::string curlGetRequest(const char* url, const char* userpwd);
	void testGetCase();
};



#else
	
enum class Protocol { None, TCP_Client, TCP_Server, UDP_Client, UDP_Server };

class KINJO_API NetworkManager {
	NetworkManager(const char* h, const char* p);
	~NetworkManager();
	virtual void ReadMessage(Game& game);		
};


#endif


#endif