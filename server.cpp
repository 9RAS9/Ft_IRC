/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: arafanom <arafanom@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/22 19:49:09 by arafanom          #+#    #+#             */
/*   Updated: 2026/10/23 01:00:19 by arafanom         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include <vector>

Server::Server(int port) : _port(port), _server_fd(-1), _addrlen(sizeof(_address))
{

}

Server::~Server()
{
}

void Server::initserver()
{
	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_server_fd == -1)
	{
		std::cerr << "create socket failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &_port, sizeof(_port)) < 0)
	{
		std::cerr << "setsockopt failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(_port);
	if (bind(_server_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
	{
		std::cerr << "bind failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (listen(_server_fd, SOMAXCONN) < 0)
	{
		std::cerr << "listen failed" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void Server::serverStart()
{
	std::vector<struct pollfd> pollfds;
	struct pollfd serverPollFd;
	serverPollFd.fd = _server_fd;
	serverPollFd.events = POLLIN;
	serverPollFd.revents = 0;
	pollfds.push_back(serverPollFd);

	while(1)
	{
		int activity = poll(&pollfds[0], pollfds.size(), -1);
		if (activity < 0)
		{
			std::cerr << "poll failed" << std::endl;
			exit(EXIT_FAILURE);
		}
		for(size_t i = 0; i < pollfds.size(); i++)
		{
			if (pollfds[i].revents & POLLIN)
			{
				if (pollfds[i].fd == _server_fd)
				{
					struct sockaddr_in clientAddress;
					int clientLen = sizeof(struct sockaddr_in);
					int clientFd = accept(_server_fd, (struct sockaddr *)&clientAddress, (socklen_t*)&clientLen);
					if (clientFd < 0)
					{
						std::cerr << "accept failed" << std::endl;
						continue;
					}
					struct pollfd clientPollFd;
					clientPollFd.fd = clientFd;
					clientPollFd.events = POLLIN;
					clientPollFd.revents = 0;
					pollfds.push_back(clientPollFd);

					std::cout << "New client connected: " << clientFd << std::endl;
				}
				else
				{
					int clientFd = pollfds[i].fd;
					char buffer[1024] = {0};
					ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
					if (bytesRead <= 0)
					{
						std::cout << "Client disconnected: " << clientFd << std::endl;
						close(clientFd);
						pollfds.erase(pollfds.begin() + i);
						i--;
					}
					else
					{
						buffer[bytesRead] = '\0';
						std::cout << "Received data from client " << clientFd << ": " << buffer << std::endl;
					}
				}
			}
		}
	}
}
