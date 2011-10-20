/**
  * libirpc - irpc_client.c
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

irpc_retval_t
usb_init(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_INIT;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;
    
    return irpc_call(func, ctx, info);
}


irpc_retval_t
usb_print_device_list(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_GET_DEVICE_LIST;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;
    
    (void)irpc_call(func, ctx, info);
    
    struct irpc_device_list devlist = info->devlist;
    
    printf("irpc_client: n_devs: %d\n", devlist.n_devs);
    
    int i = 0;
    for (; i < devlist.n_devs; i++) {
        irpc_device dev = devlist.devs[i];
        
        printf("irpc_client: [%d] bus_number: %d\n", i, dev.bus_number);
        printf("irpc_client: [%d] device_address: %d\n", i, dev.device_address);
        printf("irpc_client: [%d] num configurations: %d\n", i, dev.num_configurations);
    }
}

static int
usb_print_device_ids(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_GET_DEVICE_DESCRIPTOR;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;
    irpc_retval_t retval = IRPC_FAILURE;
    
    struct irpc_device_list devlist = info->devlist;
    info->dev = devlist.devs[0];
        
    retval = irpc_call(func, ctx, info);
    if (retval == IRPC_FAILURE)
        goto done;
        
    struct irpc_device_descriptor desc = info->desc;
        
    printf("irpc_client: idVendor:  %04x\n", desc.idVendor);
    printf("irpc_client: idProduct: %04x\n", desc.idProduct);
    
done:
    return retval;
}

static void
usb_exit(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_EXIT;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;
    
    irpc_call(func, ctx, info);
}

static void
usb_open_device_with_vid_pid(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_OPEN_DEVICE_WITH_VID_PID;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;
    
    info->vendor_id = 0x05ac;
    info->product_id = 0x8005;
    
    irpc_call(func, ctx, info);
    
    printf("irpc_client: bus_number: %d\n", info->handle.dev.bus_number);
    printf("irpc_client: device_address: %d\n", info->handle.dev.device_address);
}

static irpc_retval_t
usb_open_device(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_OPEN;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;
    
    info->dev = info->devlist.devs[0];
    
    return irpc_call(func, ctx, info);
}

static irpc_retval_t
usb_claim_interface(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_CLAIM_INTERFACE;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;

    info->intf = 0;
    
    return irpc_call(func, ctx, info);
}

static irpc_retval_t
usb_release_interface(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_RELEASE_INTERFACE;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;
    
    info->intf = 0;
    
    return irpc_call(func, ctx, info);
}

static void
usb_close(struct irpc_info *info)
{
    irpc_func_t func = IRPC_USB_CLOSE;
    irpc_context_t ctx = IRPC_CONTEXT_CLIENT;

    irpc_call(func, ctx, info);
}

int main(int argc, char **argv)
{
    irpc_retval_t retval = IRPC_FAILURE;
    struct irpc_info info;
    int port;
    
    bzero(&info, sizeof(struct irpc_info));

    if (argc < 3) {
        printf("irpc_client: ip port\n");
        return retval;
    }
    
    sscanf(argv[2], "%d", &port);
    info.ci.server_sock = connect_or_die(argv[1], port);
    
    retval = usb_init(&info);
    if (retval < 0) {
        printf("irpc_client: usb_init failed\n");
        return retval;
    }
    
    usb_print_device_list(&info);
    
    retval = usb_print_device_ids(&info);
    if (retval < 0) {
        printf("irpc_client: usb_print_device_ids failed\n");
        usb_exit(&info);
        return retval;
    }
    
    //usb_open_device_with_vid_pid(&info);
    
    retval = usb_open_device(&info);
    if (retval < 0) {
        printf("irpc_client: usb_open failed\n");
        usb_exit(&info);
        return retval;
    }
    
    retval = usb_claim_interface(&info);
    if (retval < 0) {
        printf("irpc_client: usb_claim_interface failed\n");
        goto exit;
    }
    
    retval = usb_release_interface(&info);
    if (retval < 0)
        printf("irpc_client: usb_release_interface failed\n");
    
exit:
    usb_close(&info);
    usb_exit(&info);
    
    return retval;
}
