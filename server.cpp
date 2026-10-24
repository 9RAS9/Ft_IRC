/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: arafanom <arafanom@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/22 19:49:09 by arafanom          #+#    #+#             */
/*   Updated: 2026/10/23 16:50:08 by arafanom         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

Server::Server(int port) : _buffers(), _pollfds(), _server_fd(-1), _port(port), _addrlen(sizeof(_address))
{
}

Server::~Server()
{
}

void Server::disconnectClient(size_t index)
{
	int client_fd = _pollfds[index].fd;
	std::cout << "Client disconnected: " << client_fd << std::endl;
	_buffers.erase(client_fd);
	close(client_fd);
	_pollfds.erase(_pollfds.begin() + index);
}

void Server::initserver()
{
	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_server_fd == -1)
	{
		std::cerr << "create socket failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	int opt = 1;
	if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::cerr << "setsockopt failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	if (fcntl(_server_fd, F_SETFL, O_NONBLOCK) == -1)
	{
		std::cerr << "fcntl failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	sockaddr_in server_adress;
	std::memset(&server_adress, 0, sizeof(server_adress));
	server_adress.sin_family = AF_INET;
	server_adress.sin_addr.s_addr = INADDR_ANY;
	server_adress.sin_port = htons(_port);
	if (bind(_server_fd, (struct sockaddr *)&server_adress, sizeof(server_adress)) < 0)
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
	struct pollfd server_poll_fd;
	server_poll_fd.fd = _server_fd;
	server_poll_fd.events = POLLIN;
	server_poll_fd.revents = 0;
	_pollfds.push_back(server_poll_fd);

	while(1)
	{
		int activity = poll(&_pollfds[0], _pollfds.size(), -1);
		if (activity < 0)
		{
			std::cerr << "poll failed" << std::endl;
			exit(EXIT_FAILURE);
		}
		for(size_t i = 0; i < _pollfds.size(); i++)
		{
			if (i > 0 && (_pollfds[i].revents & (POLLHUP | POLLERR| POLLNVAL)))
			{
				disconnectClient(i);
				i--;
				continue;
			}
			if (_pollfds[i].revents & POLLIN)
			{
				if (_pollfds[i].fd == _server_fd)
				{
					struct sockaddr_in clientAddress;
					socklen_t clientLen = sizeof(struct sockaddr_in);
					int client_fd = accept(_server_fd, (struct sockaddr *)&clientAddress, &clientLen);
					if (client_fd < 0)
					{
						std::cerr << "accept failed" << std::endl;
						continue;
					}
					if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1)
					{
						std::cerr << "fcntl failed" << std::endl;
						close(client_fd);
						continue;
					}
					struct pollfd client_poll_fd;
					client_poll_fd.fd = client_fd;
					client_poll_fd.events = POLLIN;
					client_poll_fd.revents = 0;
					_pollfds.push_back(client_poll_fd);
					_buffers[client_fd] = "";

					std::cout << "New client connected: " << client_fd << std::endl;
				}
				else
				{
					int client_fd = _pollfds[i].fd;
					char buffer[1024] = {0};
					ssize_t bytesRead = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
					if (bytesRead < 0)
					{
						continue;
					}
					else if (bytesRead == 0)
					{
						disconnectClient(i);
						i--;
					}
					else
					{
						_buffers[client_fd].append(buffer, bytesRead);
						size_t pos;
						while ((pos = _buffers[client_fd].find('\n')) != std::string::npos)
						{
							std::string line = _buffers[client_fd].substr(0, pos);
							_buffers[client_fd].erase(0, pos + 1);
							if (!line.empty() && line[line.size() - 1] == '\r')
							{
								line.erase(line.size() - 1);
							}
							std::cout << "Command received from client " << client_fd << ": " << line << std::endl;
						}
					}
				}
			}
		}
	}
}
