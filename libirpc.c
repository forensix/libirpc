/* -----------------------------------------------------------------------------
 *  libirpc.c
 *  libirpc
 *
 *  Created by Manuel Gebele on 16.10.11.
 *  Copyright 2011 Manuel Gebele. All rights reserved.
 * -------------------------------------------------------------------------- */

#include "libirpc.h"
#include <stdio.h>
#include <strings.h>

#include <libusb-1.0/libusb.h>
#include "libusbi.h"

#include "tpl.h"

// TODO: Add context handler for multible clients.
static libusb_context *irpc_ctx = NULL;

// -----------------------------------------------------------------------------
#pragma mark Packet Type
// -----------------------------------------------------------------------------

void
irpc_send_packet_type(irpc_usb_type type, int sock)
{
    tpl_node *tn = tpl_map("i", &type);
	tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, sock);
    tpl_free(tn);	
}

// -----------------------------------------------------------------------------
#pragma mark libusb_init
// -----------------------------------------------------------------------------

irpc_usb_retval
irpc_usb_init_recv(irpc_usb_connection_info *conn_info) 
{
    irpc_usb_type type = IRPC_USB_INIT;
    irpc_send_packet_type(type, conn_info->server_sock);

    tpl_node *tn;
    irpc_usb_retval retval;
    tn = tpl_map("i", &retval);
    tpl_load(tn, TPL_FD, conn_info->server_sock);
    tpl_unpack(tn, 0);
    tpl_free(tn);
    
    return retval;
}

void
irpc_usb_init_send(irpc_usb_connection_info *conn_info)
{
    tpl_node *tn;
    irpc_usb_retval retval = libusb_init(&irpc_ctx);
    tn = tpl_map("i", &retval);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, conn_info->client_sock);
    tpl_free(tn);
}

irpc_usb_retval
irpc_usb_init(irpc_usb_context ctx,
              irpc_usb_connection_info *conn_info)
{
    if (ctx == IRPC_USB_CONTEXT_SERVER) {
        irpc_usb_init_send(conn_info);
        return 0;
    }
    
    return irpc_usb_init_recv(conn_info);
}

// -----------------------------------------------------------------------------
#pragma mark libusb_get_device_list
// -----------------------------------------------------------------------------

void
irpc_usb_get_device_list_recv(irpc_usb_connection_info *conn_info,
                              irpc_usb_device_list *devlist)
{
    irpc_usb_type type = IRPC_USB_GET_DEVICE_LIST;
    irpc_send_packet_type(type, conn_info->server_sock);
    
    tpl_node *tn;
    tn = tpl_map("iS(iiii)#",
                 &devlist->n_devs,
                 devlist->devs,
                 IRPC_MAX_DEVS);
    tpl_load(tn, TPL_FD, conn_info->server_sock);
	tpl_unpack(tn, 0);
    tpl_free(tn);
}

void
irpc_usb_get_device_list_send(irpc_usb_connection_info *conn_info)
{
    libusb_device **list;
    irpc_usb_device_list devlist;
    
    ssize_t cnt = libusb_get_device_list(irpc_ctx, &list);
    
    bzero(&devlist, sizeof(irpc_usb_device_list));
    
    int i;
    for (i = 0; i < cnt && i < IRPC_MAX_DEVS; i++)
    {
        libusb_device *device = list[i];
        devlist.devs[i].bus_number = device->bus_number;
        devlist.devs[i].device_address = device->device_address;
        devlist.devs[i].num_configurations = device->num_configurations;
        devlist.devs[i].session_data = device->session_data;
        devlist.n_devs++;        
    }
    
    tpl_node *tn;
    tn = tpl_map("iS(iiii)#",
                 &devlist.n_devs,
                 devlist.devs,
                 IRPC_MAX_DEVS);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, conn_info->client_sock);
	tpl_free(tn);
}

void
irpc_usb_get_device_list(irpc_usb_context ctx,
                         irpc_usb_connection_info *conn_info,
                         irpc_usb_device_list *devlist)
{
    if (ctx == IRPC_USB_CONTEXT_SERVER)
        irpc_usb_get_device_list_send(conn_info);
    else
        irpc_usb_get_device_list_recv(conn_info, devlist);
}

// -----------------------------------------------------------------------------
#pragma mark libusb_get_device_descriptor
// -----------------------------------------------------------------------------

irpc_usb_retval
irpc_usb_get_device_descriptor_recv(irpc_usb_connection_info *conn_info,
                                    irpc_usb_device *dev,
                                    irpc_usb_device_descriptor *desc)
{
    irpc_usb_type type = IRPC_USB_GET_DEVICE_DESCRIPTOR;
    irpc_send_packet_type(type, conn_info->server_sock);
    
    tpl_node *tn;
    tn = tpl_map("S(iiii)", dev);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, conn_info->server_sock);
	tpl_free(tn);
    
    tn = tpl_map("S(iiiiiiiiiiiiii)", desc);
    tpl_load(tn, TPL_FD, conn_info->server_sock);
    tpl_unpack(tn, 0);
	tpl_free(tn);
}

libusb_device *
libusb_device_with_session_data(unsigned long session_data)
{
    libusb_device **list;
    libusb_device *f = NULL;
    ssize_t cnt = libusb_get_device_list(irpc_ctx, &list);
    
    int i;
    for (i = 0; i < cnt; i++)
    {
        libusb_device *dev = list[i];
        if (dev->session_data == session_data)
        {
            f = dev;
            goto found;
        }
    }

    printf("libirpc: Warning device not found (session_data: %lu\n",
           session_data);
found:
    return f;
}

void
irpc_usb_get_device_descriptor_send(irpc_usb_connection_info *conn_info)
{
    irpc_usb_device idev;
    
    // Get irpc_usb_device struct
    tpl_node *tn;
    tn = tpl_map("S(iiii)", &idev);
    tpl_load(tn, TPL_FD, conn_info->client_sock);
    tpl_unpack(tn, 0);
	tpl_free(tn);
    
    struct libusb_device_descriptor desc;
    libusb_device *dev = libusb_device_with_session_data(idev.session_data); 
    
    // TODO: retval
    libusb_get_device_descriptor(dev, &desc);
    
    irpc_usb_device_descriptor idesc;
    idesc.bLength = desc.bLength;
    idesc.bDescriptorType = desc.bDescriptorType;
    idesc.bcdUSB = desc.bcdUSB;
    idesc.bDeviceClass = desc.bDeviceClass;
    idesc.bDeviceSubClass = desc.bDeviceSubClass;
    idesc.bDeviceProtocol = desc.bDeviceProtocol;
    idesc.bMaxPacketSize0 = desc.bMaxPacketSize0;
    idesc.idVendor = desc.idVendor;
    idesc.idProduct = desc.idProduct;
    idesc.bcdDevice = desc.bcdDevice;
    idesc.iManufacturer = desc.iManufacturer;
    idesc.iProduct = desc.iProduct;
    idesc.iSerialNumber = desc.iSerialNumber;
    idesc.bNumConfigurations = desc.bNumConfigurations;
    
    tn = tpl_map("S(iiiiiiiiiiiiii)", &idesc);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, conn_info->client_sock);
	tpl_free(tn);    
}

void
irpc_usb_get_device_descriptor(irpc_usb_context ctx,
                               irpc_usb_connection_info *conn_info,
                               irpc_usb_device *dev,
                               irpc_usb_device_descriptor *desc)
{
    if (ctx == IRPC_USB_CONTEXT_SERVER)
        irpc_usb_get_device_descriptor_send(conn_info);
    else
        irpc_usb_get_device_descriptor_recv(conn_info, dev, desc);
}

// -----------------------------------------------------------------------------
#pragma mark Public API
// -----------------------------------------------------------------------------

irpc_usb_retval
irpc_usb_call(irpc_usb_type type,
              irpc_usb_context ctx,
              irpc_usb_connection_info *conn_info,
              irpc_usb_device_list *devlist,
              irpc_usb_device *dev,
              irpc_usb_device_descriptor *desc)
{
    irpc_usb_retval retval = IRPC_RETVAL_SUCCESS;
    
    switch (type) {
    case IRPC_USB_INIT:
        retval = irpc_usb_init(ctx, conn_info);
        break;
    case IRPC_USB_GET_DEVICE_LIST:
        (void)irpc_usb_get_device_list(ctx, conn_info, devlist);
        break;
    case IRPC_USB_GET_DEVICE_DESCRIPTOR:
        (void)irpc_usb_get_device_descriptor(ctx, conn_info, dev, desc);
        break;
    default:
        retval = IRPC_RETVAL_FAILURE;
        break;
    }

    return retval;
}
