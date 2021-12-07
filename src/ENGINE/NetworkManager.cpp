#include "NetworkManager.h"
#include "Entity.h"
#include "Game.h"

#include <curl/curl.h>
#include <algorithm>
#include <stdlib.h>
#include <nlohmann/json.hpp>

void NetworkManager::Init(Protocol newProtocol)
{
    protocol = newProtocol;

    if (SDL_Init(0) == -1)
    {
        printf("SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }

    if (SDLNet_Init() == -1)
    {
        printf("SDLNet_Init: %s\n", SDLNet_GetError());
        exit(2);
    }

    switch (protocol)
    {
    case Protocol::TCP_Client:
        /* Resolve the host we are connecting to */
        if (SDLNet_ResolveHost(&ipAddress, host, atoi(port)) < 0)
        {
            fprintf(stderr, "SDLNet_ResolveHost: %s\n", SDLNet_GetError());
            exit(EXIT_FAILURE);
        }

        /* Open a connection with the IP provided (listen on the host's port) */
        if (!(tcpSocket = SDLNet_TCP_Open(&ipAddress)))
        {
            fprintf(stderr, "SDLNet_TCP_Open: %s\n", SDLNet_GetError());
            exit(EXIT_FAILURE);
        }

        printf("Starting tcp client...\n");
        break;
    case Protocol::TCP_Server:

        /* Resolving the host using NULL make network interface to listen */
        if (SDLNet_ResolveHost(&ipAddress, NULL, std::stoi(port)) < 0) { fprintf(stderr, "SDLNet_ResolveHost: %s\n", SDLNet_GetError()); exit(EXIT_FAILURE); }

        /* Open a connection with the IP provided (listen on the host's port) */
        if (!(tcpSocket = SDLNet_TCP_Open(&ipAddress))) { fprintf(stderr, "SDLNet_TCP_Open: %s\n", SDLNet_GetError()); exit(EXIT_FAILURE); }

        printf("Starting tcp server...\n");
        break;
    case Protocol::UDP_Client:
        /* Open a socket on random port */
        if (!(udpSocket = SDLNet_UDP_Open(0)))
        {
            fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
            exit(EXIT_FAILURE);
        }

        /* Resolve server name */
        if (SDLNet_ResolveHost(&serverAddress, host, atoi(port)))
        {
            fprintf(stderr, "SDLNet_ResolveHost(%s %d): %s\n", host, atoi(port), SDLNet_GetError());
            exit(EXIT_FAILURE);
        }

        /* Allocate memory for the packet */
        if (!(udpPacket = SDLNet_AllocPacket(512)))
        {
            fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
            exit(EXIT_FAILURE);
        }

        printf("Starting udp client...\n");
        break;
    case Protocol::UDP_Server:
        /* Open a socket */
        if (!(udpSocket = SDLNet_UDP_Open(std::stoi(port))))
        {
            fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
            exit(EXIT_FAILURE);
        }

        /* Make space for the packet */
        if (!(udpPacket = SDLNet_AllocPacket(512)))
        {
            fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
            exit(EXIT_FAILURE);
        }

        printf("Starting udp server...\n");
        break;
    default:
        break;
    }


    // Read in the variables into the map
    // We need these keys to be added in the same consistent order across all clients
    // Therefore they should all be done once at the beginning
    mapKeysToIndices = MapStringsToLineFromFile("data/network/vars.dat");
    mapEntitiesToIndices = MapStringsToLineFromFile("data/network/entities.dat");

    mapIndicesToKeys = ReadStringsFromFile("data/network/vars.dat", true);
    mapIndicesToEntities = ReadStringsFromFile("data/network/entities.dat", true);
}

NetworkManager::NetworkManager(const char* h, const char* p)
{
    host = h;
    port = p;
}

NetworkManager::~NetworkManager()
{
    switch (protocol)
    {
    case Protocol::TCP_Client:
    case Protocol::TCP_Server:
        SDLNet_TCP_Close(tcpSocket);
        break;
    case Protocol::UDP_Client:
    case Protocol::UDP_Server:
        SDLNet_FreePacket(udpPacket);
        break;
    default:
        break;
    }

    SDLNet_Quit();
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string NetworkManager::curlPostRequest(const char* url, const char* data, const char* userpwd)
{
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {

        curl_easy_setopt(curl, CURLOPT_URL, url);

        struct curl_slist* headers = NULL; // init to NULL is important 

        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charset: utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);    // set user name and password for the authentication

        //const char* escaped_data = curl_easy_escape(curl, data, 0);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(data));

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);     // enable verbose for easier tracing

        res = curl_easy_perform(curl);
        // we have to call twice, first call authenticates, second call does the work
        //res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            const std::string strErrorDescription = "Curl call to server failed";
            std::cout << strErrorDescription << std::endl;
            std::cout << res << std::endl;
            return "";
        }

        curl_easy_cleanup(curl);

        std::cout << readBuffer << std::endl;

        return readBuffer;
    }
    else
    {
        std::cout << "Failed to init curl" << std::endl;
        return "";
    }
}


std::string NetworkManager::curlGetRequest(const char* url, const char* userpwd)
{
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {

        curl_easy_setopt(curl, CURLOPT_URL, url);

        struct curl_slist* headers = NULL; // init to NULL is important 

        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charset: utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);    // set user name and password for the authentication

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);     // enable verbose for easier tracing

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            const std::string strErrorDescription = "Curl call to server failed";
            std::cout << strErrorDescription << std::endl;
            std::cout << res << std::endl;
            return "";
        }

        curl_easy_cleanup(curl);

        std::cout << readBuffer << std::endl;

        return readBuffer;
    }
    else
    {
        std::cout << "Failed to init curl" << std::endl;
        return "";
    }
}

void NetworkManager::testGetCase()
{
    std::string curlResponse = curlGetRequest("http://localhost:3000/api/cases", "");
    if (curlResponse != "")
    {
        nlohmann::json res = nlohmann::json::parse(curlResponse);

        if (res.contains("result"))
        {
            std::string fixedResult = res["result"];

            auto noSpaceEnd = std::remove(fixedResult.begin(), fixedResult.end(), '\"');
            fixedResult.erase(noSpaceEnd, fixedResult.end());

            std::cout << "RESULT: \n======\n" << fixedResult << std::endl;
        }
    }
}


void NetworkManager::Update()
{
    switch (protocol)
    {
    case Protocol::TCP_Client:
        TCPClient();
        break;
    case Protocol::TCP_Server:
        TCPServer();
        break;
    case Protocol::UDP_Client:
        UDPClient();
        break;
    case Protocol::UDP_Server:
        UDPServer();
        break;
    default:
        break;
    }
}

void NetworkManager::TCPClient()
{
    printf("Write something:\n>");
    scanf("%s", buffer);

    len = strlen(buffer) + 1;
    if (SDLNet_TCP_Send(tcpSocket, (void*)buffer, len) < len)
    {
        fprintf(stderr, "SDLNet_TCP_Send: %s\n", SDLNet_GetError());
        exit(EXIT_FAILURE);
    }
}

void NetworkManager::TCPServer()
{
    /* This checks the tcpSocket if there is a pending connection.
     * If there is one, accept that, and open a new socket for communicating */
    if ((clientSocket = SDLNet_TCP_Accept(tcpSocket)))
    {
        /* Now we can communicate with the client using clientSocket
        * tcpSocket will remain opened waiting other connections */

        /* Get the remote address */
        if ((remoteIP = SDLNet_TCP_GetPeerAddress(clientSocket))) /* Print the address, converting in the host format */
            printf("Host connected: %x %d\n", SDLNet_Read32(&remoteIP->host), SDLNet_Read16(&remoteIP->port));
        else
            fprintf(stderr, "SDLNet_TCP_GetPeerAddress: %s\n", SDLNet_GetError());

        while (!quit)
        {
            if (SDLNet_TCP_Recv(clientSocket, buffer, 512) > 0)
            {
                printf("Client says: %s\n", buffer);
            }
        }

        /* Close the client socket */
        SDLNet_TCP_Close(clientSocket);
    }
}

void NetworkManager::UDPClient()
{
   

    //std::cin >> entity->name;
    //messageToSend = ConvertToData(*entity);

    if (messageToSend != "")
    {
        std::cout << "Sending message: " << messageToSend << std::endl;
        char* cstr = new char[messageToSend.length() + 1];
        strcpy(cstr, messageToSend.c_str());

        udpPacket->data = (Uint8*)cstr;

        udpPacket->address.host = serverAddress.host; /* Set the destination host */
        udpPacket->address.port = serverAddress.port; /* And destination port */

        udpPacket->len = strlen((char*)udpPacket->data) + 1;
        SDLNet_UDP_Send(udpSocket, -1, udpPacket); /* This sets the p->channel */

        messageToSend = "";
        delete[] cstr;
    }


}

void NetworkManager::UDPServer()
{
    messageReceived = "";

    /* Wait for a packet. UDP_Recv returns != 0 if a packet is coming */
    if (SDLNet_UDP_Recv(udpSocket, udpPacket))
    {
        printf("UDP Packet incoming\n");
        printf("\tChan: %d\n", udpPacket->channel);
        printf("\tData: %s\n", (char*)udpPacket->data);
        printf("\tLen: %d\n", udpPacket->len);
        printf("\tMaxlen: %d\n", udpPacket->maxlen);
        printf("\tStatus: %d\n", udpPacket->status);
        printf("\tAddress: %x %x\n", udpPacket->address.host, udpPacket->address.port);

        messageReceived.assign((char*)udpPacket->data, udpPacket->len);

        //Entity newEntity = ConvertFromData((char*)udpPacket->data, udpPacket->len);
        //std::cout << "Entity: " << newEntity.name << " at " << newEntity.position.x << "," << newEntity.position.y << std::endl;
    }
}

void NetworkManager::ReadMessage(Game& game)
{
    // Leave this empty, fill it out in the actual game
}

std::string NetworkManager::ConvertToData(const Entity& entity)
{
    std::string result = "";
    result += "entity;";
    result += entity.name + ";";
    result += std::to_string(entity.position.x) + ";";
    result += std::to_string(entity.position.y) + ";";
    return result;
}

Entity NetworkManager::ConvertFromData(const char* data, int len)
{
    Entity newEntity(glm::vec3(0,0,0));
    int section = 0;
    std::string word = "";
    for (int i = 0; i < len; i++)
    {
        if (data[i] == ';')
        {
            switch (section)
            {
            case 1:
                newEntity.name = word;
                break;
            case 2:
                newEntity.position.x = std::stoi(word);
                break;
            case 3:
                newEntity.position.y = std::stoi(word);
                break;
            case 0:
            default:
                // type = entity
                break;
            }
            section++;
            word = "";
        }
        else
        {
            word += data[i];
        }
    }

    return newEntity;
}

void NetworkManager::AddToMessage(const std::string& entity, int id, const std::string& key, int value, bool condition)
{
    if (condition)
    {
        messageToSend.push_back(mapEntitiesToIndices[entity]);
        messageToSend.push_back(id);
        messageToSend.push_back(mapKeysToIndices[key]);
        messageToSend.push_back(value);
    }
}