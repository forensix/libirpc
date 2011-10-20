/**
  * libirpc - libirpc.h
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

#define IRPC_MAX_DEVS 256           /* Max 256 devices */

/* Identifies the function call. */
enum irpc_func {
    IRPC_USB_INIT,                      /* libusb_init */
    IRPC_USB_EXIT,                      /* libusb_exit */
    IRPC_USB_GET_DEVICE_LIST,           /* libusb_get_device_list */
    IRPC_USB_GET_DEVICE_DESCRIPTOR,     /* libusb_get_device_descriptor */
    IRPC_USB_OPEN_DEVICE_WITH_VID_PID,  /* libusb_open_device_with_vid_pid */
};

enum irpc_context {
    IRPC_CONTEXT_CLIENT,                /* Client -> Server */
    IRPC_CONTEXT_SERVER,                /* Server -> Client */
};

typedef enum irpc_retval {
    IRPC_FAILURE = -1,                  /* Function call was success */
    IRPC_SUCCESS,                       /* Function call has failed */
} irpc_retval_t;

/* Holds connection specific information. */
struct irpc_connection_info {
    int client_sock;                    /* Client socked fd */
    int server_sock;                    /* Server socket fd */
};

/* Reflection of libusb_device. */
typedef struct {
    int bus_number;
    int device_address;
    int num_configurations;
    int session_data; // Just an identifier.
} irpc_device;

/* Reflection of libusb_device **. */
struct irpc_device_list {
    int n_devs;
    irpc_device devs[IRPC_MAX_DEVS];
};

/* Reflection of libusb_device_descriptor. */
struct irpc_device_descriptor {
    irpc_retval_t retval; 
    int bLength;
    int bDescriptorType;
    int bcdUSB;
    int bDeviceClass;
    int bDeviceSubClass;
    int bDeviceProtocol;    
    int bMaxPacketSize0;    
    int idVendor;    
    int idProduct;    
    int bcdDevice;    
    int iManufacturer;    
    int iProduct;    
    int iSerialNumber;    
    int bNumConfigurations;
};

/* Reflection of libusb_device_handle. */
typedef struct {
    irpc_device dev;
} irpc_device_handle;

struct irpc_info {
    struct irpc_connection_info ci;
    irpc_device dev;
    struct irpc_device_list devlist;
    struct irpc_device_descriptor desc;
    irpc_device_handle handle;
    int vendor_id;
    int product_id;
};

typedef enum irpc_func irpc_func_t;
typedef enum irpc_context irpc_context_t;

irpc_retval_t
irpc_call(irpc_func_t func, irpc_context_t ctx, struct irpc_info *info);
