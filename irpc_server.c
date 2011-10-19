/**
  * libirpc - irpc_server.c
  * Copyright (C) 2010 Manuel Gebele
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "libirpc.h"

static int
init_connection_or_die()
{
    int sockfd;
	struct sockaddr_in self;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		exit(1);
    
    bzero(&self, sizeof(self));
    
	self.sin_family = AF_INET;
	self.sin_port = htons(9999);
	self.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(sockfd, (struct sockaddr *)&self, sizeof(self)) != 0)
		exit(1);
    
    if (listen(sockfd, 20) != 0)
        exit(1);
    
    int tmp;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof tmp) != 0)
        exit(1);
    
    return sockfd;
}

static void
server_loop(int sock)
{
    struct irpc_info info;
    fd_set sockset;
    
    bzero(&info, sizeof(struct irpc_info));
    
    FD_ZERO(&sockset);
    FD_SET(sock, &sockset);
    
    if (select(sock + 1, &sockset, NULL, NULL, NULL) == -1) {
        fprintf(stderr, "Error! select()\n");
        return;
    }
    
    info.ci.client_sock = accept(sock, NULL, NULL);
    if (info.ci.client_sock == -1) {
        fprintf(stderr, "Error! Failed to accept an incoming connection.\n");
        return;
    }
    
    while (1) {
        irpc_func_t func = irpc_read_func(info.ci.client_sock);
        irpc_call(func, IRPC_CONTEXT_SERVER, &info);

        if (func == IRPC_USB_GET_DEVICE_DESCRIPTOR)
            goto done;
	}
    
done:
    close(info.ci.client_sock);
}

int main(int argc, char *argv[])
{   
    int sock;
    
    sock = init_connection_or_die();
    server_loop(sock);
    
    close(sock);
    
	return 0;
}