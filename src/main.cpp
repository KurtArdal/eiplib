#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#elif _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#endif

#include <string>
#include <iostream>


class Net {
public:
	bool connects(std::string url, std::string port);
	bool disconnect();

	bool sendMessage(std::string message);
	bool recvMessage();

private:
	int iResult; // Used for checking status.
	#ifdef _WIN32
	WSADATA wsaData;

	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;

	SOCKET connSock = INVALID_SOCKET;
	#elif __linux__
	int sockfd;
	int portno;

    struct sockaddr_in serv_addr;
    struct hostent *server;
	#endif

	char buffer[64000];

};

bool Net::connects(std::string url, std::string port) {
	#ifdef __linux__
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		std::cout << "Error opening socket" << std::endl;
		return false;
	}

	server = gethostbyname(url.c_str());
	if (server == NULL) {
		std::cout << "Error, no such host" << std::endl;
		return false;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons((atoi(port.c_str())));

	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Error connection" << std::endl;
		return false;
	}
	#elif _WIN32
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != 0) {
		std::cout << "WSAStartup failed: " << iResult << std::endl;
		return false;
	}
	
	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(url.c_str(), port.c_str(), &hints, &result);

	if (iResult != 0) {
		std::cout << "Getaddrinfo failed: " << iResult << std::endl;
		WSACleanup();
		return false;
	}

	ptr = result;

	connSock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

	if (connSock == INVALID_SOCKET) {
		std::cout << "Error at socket(): " << WSAGetLastError << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	// Connect to server.
	iResult = connect(connSock, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(connSock);
		connSock = INVALID_SOCKET;
	}
	freeaddrinfo(result);

	if (connSock == INVALID_SOCKET) {
		std::cout << "Unable to connect to server!" << std::endl;
		WSACleanup();
		return false;
	}
	#endif

	return true;
}

bool Net::disconnect() {
	#ifdef __linux__
		close(sockfd);
	#elif _WIN32
		// shutdown the send half of the connection since no more data will be sent
		iResult = shutdown(connSock, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed: %d\n", WSAGetLastError());
			closesocket(connSock);
			WSACleanup();
			return false;
		}
	#endif

	return true;
}

bool Net::sendMessage(std::string message) {
	#ifdef __linux__
	iResult = send(sockfd, message.c_str(), strlen(message.c_str()), 0);
    if (iResult < 0) {
         std::cout << "Error writing to socket" << std::endl;
		 return false;
	}
	#elif _WIN32
	iResult = send(connSock, message.c_str(), (int)strlen(message.c_str()), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(connSock);
		WSACleanup();
		return false;
	}
	#endif
	return true;
}

bool Net::recvMessage() {
	//bzero(buffer,64000);

	int bytes = 0;
    do {
		iResult = recv(sockfd, buffer, 64000, 0);
		if (iResult > 0) {
			//std::cout << buffer << std::endl;
			bytes += iResult;
			std::cout << "Bytes received: " << iResult << std::endl;
		}
		else if (iResult == 0) {
			std::cout << "Connection closed\n" << std::endl;
		}
		else {		 
			std::cout << "recv failed:"  << std::endl;
		}
	} while (iResult > 0);
	

	
	std::cout << bytes << std::endl;
    std::cout << buffer << std::endl;

	return true;
}

int main()
{
    Net net;

	std::string url = "www.google.no";
	std::string port = "80";

	net.connects(url, port);

    std::string getReq = "GET /index.html HTTP/1.1\r\nHost: " + url + "\r\nConnection: close\r\n\r\n";
    
	net.sendMessage(getReq);

	net.recvMessage();

	net.disconnect();
    
	std::cin.get();
    
    return 0;
}
