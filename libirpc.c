/**
  * libirpc - libirpc.c
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

#include "libirpc.h"

#include <stdio.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include "libusbi.h"
#include "tpl.h"

// TODO: Add context handler for multiple clients.
static libusb_context *irpc_ctx = NULL;

// TODO: Add device handler for multiple clients.
static struct libusb_device_handle *irpc_handle = NULL; 

static int dbgmsg = 1;

#define dbgmsg(fmt, arg...) \
 do {                       \
  if (dbgmsg)               \
   printf(fmt, ##arg);      \
 } while (0)

#define IRPC_INT_FMT            "i"
#define IRPC_DEV_FMT            "S(iiii)"
#define IRPC_DEVLIST_FMT        "iS(iiii)#"
#define IRPC_DESC_FMT           "S(iiiiiiiiiiiiii)i"    // retval
#define IRPC_PRID_VEID_FMT      "ii"
#define IRPC_DEV_HANDLE_FMT     "S($(iiii))"
#define IRPC_DEV_HANDLE_RET_FMT "S($(iiii))i"           // retval

// -----------------------------------------------------------------------------
#pragma mark Function Call Identification
// -----------------------------------------------------------------------------

void
irpc_send_func(irpc_func_t func, int sock)
{
    tpl_node *tn = tpl_map(IRPC_INT_FMT, &func);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, sock);
    tpl_free(tn);	
}

irpc_func_t
irpc_read_func(int sock)
{
    irpc_func_t func;
    tpl_node *tn = tpl_map(IRPC_INT_FMT, &func);
    tpl_load(tn, TPL_FD, sock);
    tpl_unpack(tn, 0);
    tpl_free(tn);
    
    return func;
}

// -----------------------------------------------------------------------------
#pragma mark libusb_init
// -----------------------------------------------------------------------------

irpc_retval_t
irpc_recv_usb_init(struct irpc_connection_info *ci)
{
    tpl_node *tn = NULL;
    irpc_retval_t retval = IRPC_FAILURE; 
    irpc_func_t func = IRPC_USB_INIT;
    int sock = ci->server_sock;
    
    irpc_send_func(func, sock);
    
    // Read usb_init packet.
    tn = tpl_map(IRPC_INT_FMT, &retval);
    tpl_load(tn, TPL_FD, sock);
    tpl_unpack(tn, 0);
    tpl_free(tn);
        
    return retval;
}

void
irpc_send_usb_init(struct irpc_connection_info *ci)
{
    tpl_node *tn = NULL;
    int sock = ci->client_sock;
    irpc_retval_t retval = libusb_init(&irpc_ctx);
        
    // Send usb_init packet.
    tn = tpl_map(IRPC_INT_FMT, &retval);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, sock);
    tpl_free(tn);
}

irpc_retval_t
irpc_usb_init(struct irpc_connection_info *ci,
              irpc_context_t ctx)
{
    irpc_retval_t retval = IRPC_SUCCESS;
    
    if (ctx == IRPC_CONTEXT_SERVER)
        (void)irpc_send_usb_init(ci);
    else
        retval = irpc_recv_usb_init(ci);
    
    return retval;
}

// -----------------------------------------------------------------------------
#pragma mark libusb_exit
// -----------------------------------------------------------------------------

void
irpc_usb_exit(struct irpc_connection_info *ci,
              irpc_context_t ctx)
{    
    if (ctx == IRPC_CONTEXT_SERVER) {
        libusb_exit(irpc_ctx);
        return;
    }
    irpc_func_t func = IRPC_USB_EXIT;
    int sock = ci->server_sock;
    
    irpc_send_func(func, sock);
    irpc_ctx = NULL;
}

// -----------------------------------------------------------------------------
#pragma mark libusb_get_device_list
// -----------------------------------------------------------------------------

void
irpc_recv_usb_get_device_list(struct irpc_connection_info *ci,
                              struct irpc_device_list *devlist)
{
    tpl_node *tn = NULL;
    irpc_func_t func = IRPC_USB_GET_DEVICE_LIST;
    int sock = ci->server_sock;
    
    irpc_send_func(func, sock);
    
    // Read usb_get_device_list packet.
    tn = tpl_map(IRPC_DEVLIST_FMT,
                 &devlist->n_devs,
                 devlist->devs,
                 IRPC_MAX_DEVS);
    tpl_load(tn, TPL_FD, sock);
    tpl_unpack(tn, 0);
    tpl_free(tn);
}

void
irpc_send_usb_get_device_list(struct irpc_connection_info *ci)
{
    tpl_node *tn = NULL;
    libusb_device **list = NULL;
    struct irpc_device_list devlist;
    int sock = ci->client_sock;
    
    bzero(&devlist, sizeof(struct irpc_device_list));
    
    int i;
    ssize_t cnt = libusb_get_device_list(irpc_ctx, &list);
    for (i = 0; i < cnt && i < IRPC_MAX_DEVS; i++) {
        libusb_device *dev = *(list + i);
        irpc_device *idev = &devlist.devs[i];
        idev->bus_number = dev->bus_number;
        idev->device_address = dev->device_address;
        idev->num_configurations = dev->num_configurations;
        idev->session_data = dev->session_data;
        devlist.n_devs++;
    }
    
    // Send usb_get_device_list packet.
    tn = tpl_map(IRPC_DEVLIST_FMT,
                 &devlist.n_devs,
                 devlist.devs,
                 IRPC_MAX_DEVS);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, sock);
    tpl_free(tn);
    
    libusb_free_device_list(list, 1);
}

void
irpc_usb_get_device_list(struct irpc_connection_info *ci,
                         irpc_context_t ctx,
                         struct irpc_device_list *devlist)
{
    if (ctx == IRPC_CONTEXT_SERVER)
        irpc_send_usb_get_device_list(ci);
    else
        irpc_recv_usb_get_device_list(ci, devlist);
}

// -----------------------------------------------------------------------------
#pragma mark libusb_get_device_descriptor
// -----------------------------------------------------------------------------

irpc_retval_t
irpc_recv_usb_get_device_descriptor(struct irpc_connection_info *ci,
                                    irpc_device *idev,
                                    struct irpc_device_descriptor *desc)
{
    tpl_node *tn = NULL;
    irpc_func_t func = IRPC_USB_GET_DEVICE_DESCRIPTOR;
    irpc_retval_t retval;
    int sock = ci->server_sock;
    
    irpc_send_func(func, sock);
    
    // Send irpc_device to server.
    tn = tpl_map(IRPC_DEV_FMT, idev);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, sock);
    tpl_free(tn);
    
    // Read libusb_get_device_descriptor packet.
    tn = tpl_map(IRPC_DESC_FMT, desc, &retval);
    tpl_load(tn, TPL_FD, sock);
    tpl_unpack(tn, 0);
    tpl_free(tn);
    
    return retval;
}

void
irpc_send_usb_get_device_descriptor(struct irpc_connection_info *ci)
{
    tpl_node *tn = NULL;
    irpc_retval_t retval = IRPC_SUCCESS;
    libusb_device *f = NULL;
    libusb_device **list = NULL;
    irpc_device idev;
    struct irpc_device_descriptor idesc;
    struct libusb_device_descriptor desc;
    int sock = ci->client_sock;
    
    bzero(&idesc, sizeof(struct irpc_device_descriptor));
    
    // Read irpc_device from client.
    tn = tpl_map(IRPC_DEV_FMT, &idev);
    tpl_load(tn, TPL_FD, sock);
    tpl_unpack(tn, 0);
    tpl_free(tn);
    
    // Find corresponding usb_device.
    int i;
    ssize_t cnt = libusb_get_device_list(irpc_ctx, &list);
    for (i = 0; i < cnt; i++) {
        libusb_device *dev = list[i];
        if (dev->session_data == idev.session_data) {
            f = dev;
            break;
        }
    }
        
    if (!f) {
        retval = IRPC_FAILURE;
        goto send;
    }
    
    if (libusb_get_device_descriptor(f, &desc) < 0) {
        retval = IRPC_FAILURE;
        goto send;
    }
    libusb_free_device_list(list, 1);
    
    // Success, build descriptor
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

send:
    // Send libusb_get_device_descriptor packet.
    tn = tpl_map(IRPC_DESC_FMT, &idesc, &retval);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, sock);
    tpl_free(tn);  
}

irpc_retval_t
irpc_usb_get_device_descriptor(struct irpc_connection_info *ci,
                               irpc_context_t ctx,
                               irpc_device *idev,
                               struct irpc_device_descriptor *desc)
{
    irpc_retval_t retval = IRPC_SUCCESS;
    
    if (ctx == IRPC_CONTEXT_SERVER)
        (void)irpc_send_usb_get_device_descriptor(ci);
    else
        retval = irpc_recv_usb_get_device_descriptor(ci, idev, desc);
    
    return retval;
}

// -----------------------------------------------------------------------------
#pragma mark libusb_open
// -----------------------------------------------------------------------------

irpc_retval_t
irpc_recv_usb_open(struct irpc_connection_info *ci,
                   irpc_device_handle *handle,
                   irpc_device *dev)
{
    tpl_node *tn = NULL;
    irpc_retval_t retval;
    irpc_func_t func = IRPC_USB_OPEN;
    int sock = ci->server_sock;
    
    irpc_send_func(func, sock);
    
    // Send irpc_device to server.
    tn = tpl_map(IRPC_DEV_FMT, dev);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, sock);
    tpl_free(tn);

    // Read libusb_open packet.
    tn = tpl_map(IRPC_DEV_HANDLE_RET_FMT, handle, &retval);
    tpl_load(tn, TPL_FD, sock);
    tpl_unpack(tn, 0);
    tpl_free(tn);
    
    return retval;
}

void
irpc_send_usb_open(struct irpc_connection_info *ci)
{
    tpl_node *tn = NULL;
    irpc_retval_t retval = IRPC_SUCCESS;
    irpc_device idev;
    libusb_device *f = NULL;
    libusb_device **list = NULL;
    irpc_device_handle ihandle;
    int sock = ci->client_sock;
        
    // Read irpc_device from client.
    tn = tpl_map(IRPC_DEV_FMT, &idev);
    tpl_load(tn, TPL_FD, sock);
    tpl_unpack(tn, 0);
    tpl_free(tn);
    
    int i;
    ssize_t cnt = libusb_get_device_list(irpc_ctx, &list);
    for (i = 0; i < cnt; i++) {
        libusb_device *dev = list[i];
        if (dev->session_data == idev.session_data) {
            f = dev;
            break;
        }
    }

    if (!f) {
        retval = IRPC_FAILURE;
        goto send;
    }
    
    // Close an already opened handle.
    if (irpc_handle) {
        libusb_close(irpc_handle);
        irpc_handle = NULL;
    }
    
    if (libusb_open(f, &irpc_handle) != 0) {
        retval = IRPC_FAILURE;
        goto send;
    }
    libusb_free_device_list(list, 1);

    ihandle.dev.bus_number = irpc_handle->dev->bus_number;
    ihandle.dev.device_address = irpc_handle->dev->device_address;
    ihandle.dev.num_configurations = irpc_handle->dev->num_configurations;
    ihandle.dev.session_data = irpc_handle->dev->session_data;
    
send:
    // Send libusb_open packet.
    tn = tpl_map(IRPC_DEV_HANDLE_RET_FMT, &ihandle, &retval);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, sock);
    tpl_free(tn);
}

irpc_retval_t
irpc_usb_open(struct irpc_connection_info *ci,
              irpc_context_t ctx,
              irpc_device_handle *handle,
              irpc_device *dev)
{
    irpc_retval_t retval = IRPC_SUCCESS;
    
    if (ctx == IRPC_CONTEXT_SERVER)
        (void)irpc_send_usb_open(ci);
    else
        retval = irpc_recv_usb_open(ci, handle, dev);
    
    return retval;
}

// -----------------------------------------------------------------------------
#pragma mark libusb_open_device_with_vid_pid
// -----------------------------------------------------------------------------

void
irpc_recv_usb_open_device_with_vid_pid(struct irpc_connection_info *ci,
                                       int vendor_id,
                                       int product_id,
                                       irpc_device_handle *handle)
{
    tpl_node *tn = NULL;
    irpc_func_t func = IRPC_USB_OPEN_DEVICE_WITH_VID_PID;
    int sock = ci->server_sock;
    
    irpc_send_func(func, sock);
    
    // Send vendor and product id to server.
    tn = tpl_map(IRPC_PRID_VEID_FMT, &vendor_id, &product_id);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, sock);
    tpl_free(tn);

    // Read libusb_open_device_with_vid_pid packet.
    tn = tpl_map(IRPC_DEV_HANDLE_FMT, handle);
    tpl_load(tn, TPL_FD, sock);
    tpl_unpack(tn, 0);
    tpl_free(tn);
}

void
irpc_send_usb_open_device_with_vid_pid(struct irpc_connection_info *ci)
{
    tpl_node *tn = NULL;
    irpc_device_handle ihandle;
    int vendor_id, product_id;
    int sock = ci->client_sock;
    
    bzero(&ihandle, sizeof(irpc_device_handle));
    
    // Read vendor and product id from client.
    tn = tpl_map(IRPC_PRID_VEID_FMT,
                 &vendor_id,
                 &product_id);
    tpl_load(tn, TPL_FD, sock);
    tpl_unpack(tn, 0);
    tpl_free(tn);
    
    // Close an already opened handle.
    if (irpc_handle) {
        libusb_close(irpc_handle);
        irpc_handle = NULL;
    }
   
    irpc_handle = libusb_open_device_with_vid_pid(irpc_ctx, vendor_id, product_id);
    if (!irpc_handle)
        goto send;
    
    ihandle.dev.bus_number = irpc_handle->dev->bus_number;
    ihandle.dev.device_address = irpc_handle->dev->device_address;
    ihandle.dev.num_configurations = irpc_handle->dev->num_configurations;
    ihandle.dev.session_data = irpc_handle->dev->session_data;
    
send:
    // Send libusb_open_device_with_vid_pid packet.
    tn = tpl_map(IRPC_DEV_HANDLE_FMT, &ihandle);
    tpl_pack(tn, 0);
    tpl_dump(tn, TPL_FD, sock);
    tpl_free(tn);
}

void
irpc_usb_open_device_with_vid_pid(struct irpc_connection_info *ci,
                                  irpc_context_t ctx,
                                  int vendor_id,
                                  int product_id,
                                  irpc_device_handle *handle)
{
    if (ctx == IRPC_CONTEXT_SERVER)
        irpc_send_usb_open_device_with_vid_pid(ci);
    else
        irpc_recv_usb_open_device_with_vid_pid(ci, vendor_id, product_id, handle);
}

// -----------------------------------------------------------------------------
#pragma mark libusb_close
// -----------------------------------------------------------------------------

void
irpc_recv_usb_close(struct irpc_connection_info *ci,
                    irpc_device_handle *ihandle)
{
    /*
     * TODO: Currently we can ignore ihandle since we hold
     * only one usb_device_handle handle.  Later we need
     * to add an identifier to the irpc_device_handle
     * structure in order to determine the correct handle.
     */
    irpc_func_t func = IRPC_USB_CLOSE;
    int sock = ci->server_sock;
    
    irpc_send_func(func, sock);
}

void
irpc_send_usb_close(struct irpc_connection_info *info)
{
    if (!irpc_handle) return;
    libusb_close(irpc_handle);
    irpc_handle = NULL;
}

void
irpc_usb_close(struct irpc_connection_info *ci,
               irpc_context_t ctx,
               irpc_device_handle *ihandle)
{
    if (ctx == IRPC_CONTEXT_SERVER)
        irpc_send_usb_close(ci);
    else
        irpc_recv_usb_close(ci, ihandle);
}

// -----------------------------------------------------------------------------
#pragma mark Public API
// -----------------------------------------------------------------------------

irpc_retval_t
irpc_call(irpc_func_t func, irpc_context_t ctx, struct irpc_info *info)
{
    irpc_retval_t retval = IRPC_SUCCESS;
    
    switch (func)
    {
    case IRPC_USB_INIT:
        retval = irpc_usb_init(&info->ci, ctx);
        break;
    case IRPC_USB_EXIT:
        (void)irpc_usb_exit(&info->ci, ctx);
        break;
    case IRPC_USB_GET_DEVICE_LIST:
        (void)irpc_usb_get_device_list(&info->ci, ctx, &info->devlist);
        break;
    case IRPC_USB_GET_DEVICE_DESCRIPTOR:
        retval = irpc_usb_get_device_descriptor(&info->ci, ctx, &info->dev, &info->desc);
        break;
    case IRPC_USB_OPEN_DEVICE_WITH_VID_PID:
        (void)irpc_usb_open_device_with_vid_pid(&info->ci, ctx, info->vendor_id, info->product_id, &info->handle);
        break;
    case IRPC_USB_CLOSE:
        (void)irpc_usb_close(&info->ci, ctx, &info->handle);
        break;
    case IRPC_USB_OPEN:
        retval = irpc_usb_open(&info->ci, ctx, &info->handle, &info->dev);
        break;
    default:
        retval = IRPC_FAILURE;
        break;
    }
    
    return retval;
}
