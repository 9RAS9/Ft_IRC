/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: arafanom <arafanom@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/22 19:37:45 by arafanom          #+#    #+#             */
/*   Updated: 2026/10/22 23:45:52 by arafanom         ###   ########.fr       */
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

class Server {
	private:
		int _server_fd;
		struct sockaddr_in _address;
		int _port;
		socklen_t _addrlen;
	public:
		Server(int port);
		~Server();
		void initserver();
		void serverStart();
};

#endif