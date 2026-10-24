/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: arafanom <arafanom@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/22 19:37:45 by arafanom          #+#    #+#             */
/*   Updated: 2026/10/23 07:07:33 by arafanom         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <poll.h>

class Server {
	private:
		std::map<int, std::string> _buffers;
		std::vector<struct pollfd> _pollfds;
		int _server_fd;
		struct sockaddr_in _address;
		int _port;
		socklen_t _addrlen;
	public:
		Server(int port);
		~Server();
		void initserver();
		void serverStart();
		void disconnectClient(size_t index);
};

#endif