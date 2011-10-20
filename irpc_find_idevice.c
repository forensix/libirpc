/**
  * libirpc - irpc_find_idevice.c
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
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libirpc.h"
#include "libirecovery.h"

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

static irpc_retval_t
usb_init(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_INIT;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;
    return irpc_call(func, ctx, info);
}

static void
usb_exit(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_EXIT;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;
    irpc_call(func, ctx, info);
}

static void
try_to_find_idevice(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_GET_DEVICE_LIST;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;
    irpc_retval_t retval = IRPC_FAILURE;
    
    irpc_call(func, ctx, info);
    
    func = IRPC_USB_GET_DEVICE_DESCRIPTOR;
    ctx = IRPC_CONTEXT_CLIENT;
    struct irpc_device_descriptor desc;
    struct irpc_device_list devlist = info->devlist;
    int i = 0;
    for (; i < devlist.n_devs; i++) {
        info->dev = devlist.devs[i];
        retval = irpc_call(func, ctx, info);
        if (retval == IRPC_FAILURE)
            return;
        desc = info->desc;
        if (desc.idVendor != APPLE_VENDOR_ID)
            continue;
        if (desc.idProduct == kRecoveryMode1 ||
            desc.idProduct == kRecoveryMode2 ||
            desc.idProduct == kRecoveryMode3 ||
            desc.idProduct == kRecoveryMode4 ||
            desc.idProduct == kDfuMode) {
            // Got apple device
            printf("[*] Found device in recovery mode (%04x:%04x)\n",
                   desc.idVendor, desc.idProduct);
            return;
        }
    }
    printf("[*] No recovery device found\n");
}

int
main(int argc, char **argv)
{
    irpc_retval_t retval = IRPC_FAILURE;
    struct irpc_info info;
    int port;
    
    bzero(&info, sizeof(struct irpc_info));
    
    if (argc < 3) {
        printf("irpc_find_idevice: ip port\n");
        return retval;
    }
    
    sscanf(argv[2], "%d", &port);
    info.ci.server_sock = connect_or_die(argv[1], port);
    
    retval = usb_init(&info);
    if (retval < 0) {
        printf("irpc_find_idevice: usb_init failed\n");
        return 1;
    }
    
    printf("[*] Looking on %s for recovery device...\n", argv[1]);
    try_to_find_idevice(&info);
    
    usb_exit(&info);
    
    return 0;
}
