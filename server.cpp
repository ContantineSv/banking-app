#include <cstdint>
#include <iostream>
#include <thread>
#include "database.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<arpa/inet.h>
#endif

#define USERS "users.dat"
#define ACCOUNTS "accounts.dat"
#define PORT "8888"


enum class Mode
{
	logged_off,
	logged_in,
	exit,
};

void sendAccounts(const Database& data, int socket, int32_t user)
{
   std::vector<int32_t> accounts = data.getUsersAccounts(user);
   uint16_t number = accounts.size();
   number = htons(number);
   send(socket, reinterpret_cast<char*>(&number), sizeof(number), 0);
   for (size_t i = 0; i < accounts.size(); ++i)
   {    
        std::cout << accounts[i] << std::endl;
        int32_t account = htonl(accounts[i]);
        send(socket, reinterpret_cast<char*>(&account), sizeof(account), 0);
   }
}

void handle (int client_socket, Database& data){
	char buff[20];
	uint16_t request = 0;
	uint16_t response = -1;
	Mode mode = Mode::logged_off;
	int32_t user = -1;

	while (mode != Mode::exit)
	{
		if (recv(client_socket, reinterpret_cast<char*>(&request), sizeof(request), 0) <= 0)
			break;
		request = ntohs(request);
		if (mode == Mode::logged_off)
		{
			
			switch (request)
			{
				// request to login
				case 1:
				{
					if (recv(client_socket, buff, 20, 0) <= 0)
						break;
					std::string login = std::string(buff);
					if (recv(client_socket, buff, 20, 0) <= 0)
						break;
					std::string password = std::string(buff);
		
					response = data.login(login, password);
					// send 0 if OK, 1 if user doesn't exist, 2 if password is invalid
					response = htons(response);
					send(client_socket, reinterpret_cast<char*>(&response), sizeof(response), 0);
					response = ntohs(response);
					if (response == 0){
						user = data.getUserByLogin(login);
						mode = Mode::logged_in;
						// send accounts
						sendAccounts(data, client_socket, user);
					}

					break;
				}
				// request to register
				case 2:
				{
					if (recv(client_socket, buff, 20, 0) <= 0)
						break;
					std::string login = std::string(buff);
					if (recv(client_socket, buff, 20, 0) <= 0)
						break;
					std::string password = std::string(buff);
					if (recv(client_socket, buff, 20, 0) <= 0)
						break;
					std::string confirm = std::string(buff);
					// try to add the user, send 0 if OK, 1 if already exists, 2 if passwords don't match
					response = data.addUser(login, password, confirm);
					response = htons(response);
					send(client_socket, reinterpret_cast<char*>(&response), sizeof(response), 0);
			 		break;
				}
				// exit
				case 3:
				{
					mode = Mode::exit;
					break;
				}
			}
		}
		else if (mode == Mode::logged_in)
		{
            switch (request){
				// exit
                case 1:{
                    mode = Mode::exit;
                    break;
                }
				// log out
				case 2:
				{
					user = -1;
					mode = Mode::logged_off;
					break;
				}
				// add account
				case 3:
				{
					data.addAccount(user);
					sendAccounts(data, client_socket, user);
					break;
				}
				// close account
				case 4: 
				{
					int32_t account_n;
					if (recv(client_socket, reinterpret_cast<char*> (&account_n), sizeof(account_n), 0) <= 0)
						break;
					account_n = ntohl(account_n);
					uint16_t response = data.closeAccount(account_n);
					response = htons(response);
					send(client_socket, reinterpret_cast<char*>(&response), sizeof(response), 0);
					sendAccounts(data, client_socket, user);
					break;
				}
            }
		}
	}
#ifdef _WIN32
	closesocket(client_socket);
#else
	close(client_socket);
#endif
	std::cout << "EXIT" << std::endl; 
}

void * get_in_addr(struct sockaddr * sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char * argv[])
{
	int status;
	struct addrinfo hints, *res;
	int listner;

#ifdef _WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		std::cout << "WSAStartup failed with error:" << iResult << std::endl;
		exit(1);
	}
#endif


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	status = getaddrinfo(NULL, PORT, &hints, &res);
	if (status != 0)
	{
		std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
	}

	// Create Socket and check if error occured afterwards
	listner = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (listner < 0)
	{
		std::cerr << "socket error " << gai_strerror(status) << std::endl;
	}

	// Bind the socket to the address of my local machine and port number 
	status = bind(listner, res->ai_addr, res->ai_addrlen);
	if (status < 0)
	{
		std::cerr << "bind: " << gai_strerror(status) << std::endl;
	}

	status = listen(listner, 10);
	if (status < 0)
	{
		std::cerr << "listen: " << gai_strerror(status) << std::endl;
	}

	// Free the res linked list after we are done with it	
	freeaddrinfo(res);


	int new_conn_fd;
	struct sockaddr_storage client_addr;
	socklen_t addr_size;
	char s[INET6_ADDRSTRLEN];

		
	addr_size = sizeof client_addr;

	// database setup
	Database& data = Database::getInstance();
	data.initialize(USERS, ACCOUNTS);
	// exit thread;
	std::thread cleanup (
		[](Database& data) {
			std::string input;
			std::cout << "Type quit to shut down the server: " << std::endl;
			while (std::cin >> input)
			{
				if (input == "quit"){
					data.store(USERS, ACCOUNTS);
					exit(0);
				}
			}
		},
		std::ref(data)
	);
	cleanup.detach();

	while (1) 
	{
		new_conn_fd = accept(listner, (struct sockaddr *) & client_addr, &addr_size);
		if (new_conn_fd < 0)
		{
			std::cerr << "accept: " << gai_strerror(status) << std::endl;
			continue;
		}

		std::thread handler (handle, new_conn_fd, std::ref(data));
		handler.detach();
	}

#ifdef _WIN32
	closesocket(new_conn_fd);
	WSACleanup();
#else
	close(new_conn_fd);
#endif

	return 0;
}