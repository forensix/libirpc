/* -----------------------------------------------------------------------------
 *  irpc_server.h
 *  libirpc
 *
 *  Created by Manuel Gebele on 16.10.11.
 *  Copyright 2011 Manuel Gebele. All rights reserved.
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "libirpc.h"

#include "tpl.h"

static int init_connection_or_die()
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

static void server_loop(int sock)
{
    irpc_usb_connection_info conn_info;
    fd_set sockset;
    
    FD_ZERO(&sockset);
    FD_SET(sock, &sockset);
    
    if (select(sock + 1, &sockset, NULL, NULL, NULL) == -1) {
        fprintf(stderr, "Error! select()\n");
        return;
    }
    
    conn_info.client_sock = accept(sock, NULL, NULL);
    if (conn_info.client_sock == -1) {
        fprintf(stderr, "Error! Failed to accept an incoming connection.\n");
        return;
    }
    
    while (1) {
        tpl_node *tn;
        irpc_usb_type type;
                
        tn = tpl_map("i", &type);
		tpl_load(tn, TPL_FD, conn_info.client_sock);
		tpl_unpack(tn, 0);
		tpl_free(tn);
                        
		switch (type) {
        case IRPC_USB_INIT:
            irpc_usb_call(type, IRPC_USB_CONTEXT_SERVER, &conn_info, NULL, NULL, NULL);
            break;
        case IRPC_USB_GET_DEVICE_LIST:
            irpc_usb_call(type, IRPC_USB_CONTEXT_SERVER, &conn_info, NULL, NULL, NULL);
            break;
        case IRPC_USB_GET_DEVICE_DESCRIPTOR:
            irpc_usb_call(type, IRPC_USB_CONTEXT_SERVER, &conn_info, NULL, NULL, NULL);
            goto done;
        default:
            break;
		}
	}
    
done:
    close(conn_info.client_sock);
}

int main(int argc, char *argv[])
{   
    int sock;

    sock = init_connection_or_die();
    server_loop(sock);
    
	return 0;
}