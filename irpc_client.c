/* -----------------------------------------------------------------------------
 *  irpc_client.h
 *  libirpc
 *
 *  Created by Manuel Gebele on 16.10.11.
 *  Copyright 2011 Manuel Gebele. All rights reserved.
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libirpc.h"

static int
create_client_socket()
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        return -1;
    
    return sock;
}

static int
try_connect(int sock, const char *ip, int port)
{
    struct sockaddr_in server;
    int success;
    
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    
    success = inet_aton(ip, &(server.sin_addr));
    if (!success)
        return -1;
    
    return connect(sock,
                   (struct sockaddr *)&server,
                   sizeof server);
}

static int
connect_or_die(const char *ip, int port)
{
    int sock = create_client_socket();
    
    if (sock == -1) {
		fprintf(stderr, "Error! Failed to create a client socket: %s\n", ip);
		exit(1);
	}
    
    if (try_connect(sock, ip, port) == -1) {
        fprintf(stderr, "Error! Failed to connect to a server at: %s\n", ip);
		exit(1);
    }
    
    return sock;
}

static int
usb_init(irpc_usb_connection_info *conn_info)
{
    return irpc_usb_call(IRPC_USB_INIT,
                         IRPC_USB_CONTEXT_CLIENT,
                         conn_info, NULL, NULL, NULL);
}

void
print_device_list(irpc_usb_connection_info *conn_info,
                  irpc_usb_device_list *devlist)
{
    irpc_usb_call(IRPC_USB_GET_DEVICE_LIST,
                  IRPC_USB_CONTEXT_CLIENT,
                  conn_info,
                  devlist,
                  NULL,
                  NULL);
    
    printf("irpc_client: no_devs: %d\n", devlist->n_devs);

    int i;
    for (i = 0; i < devlist->n_devs; i++) {
        irpc_usb_device dev = devlist->devs[i];
        printf("irpc_client: [%d] bus_number %d\n", i, dev.bus_number);
        printf("irpc_client: [%d] device add %d\n", i, dev.device_address);
        printf("irpc_client: [%d] num config %d\n", i, dev.num_configurations);
    }
}

void
print_device_descriptor(irpc_usb_connection_info *conn_info,
                        irpc_usb_device *dev)
{
    irpc_usb_device_descriptor desc;
    irpc_usb_call(IRPC_USB_GET_DEVICE_DESCRIPTOR,
                  IRPC_USB_CONTEXT_CLIENT,
                  conn_info, NULL, dev, &desc);
    printf("irpc_client: idVendor:  %04x\n", desc.idVendor);
    printf("irpc_client: idProduct: %04x\n", desc.idProduct);
}

int
main(int argc, char **argv)
{
    irpc_usb_connection_info conn_info;
    irpc_usb_device_list devlist;
    int port;
    
	if (argc < 2) {
		printf("irpc_client: ip port\n");
		return 1;
	}
    
    bzero(&conn_info, sizeof(conn_info));
    bzero(&devlist, sizeof(devlist));
    
    sscanf(argv[2], "%d", &port);
    conn_info.server_sock = connect_or_die(argv[1], port);

    usb_init(&conn_info);
    print_device_list(&conn_info, &devlist);
    print_device_descriptor(&conn_info, &devlist.devs[0]);
    
    return 0;
}
