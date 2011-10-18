/* -----------------------------------------------------------------------------
 *  libirpc.h
 *  libirpc
 *
 *  Created by Manuel Gebele on 16.10.11.
 *  Copyright 2011 Manuel Gebele. All rights reserved.
 * -------------------------------------------------------------------------- */

#include <stdint.h>

#define IRPC_MAX_DEVS 256           /* Max 256 devices */

/* Identifies the function call. */
enum irpc_usb_type {
    IRPC_USB_INIT,                  /* libusb_init */
    IRPC_USB_GET_DEVICE_LIST,       /* libusb_get_device_list */
    IRPC_USB_GET_DEVICE_DESCRIPTOR, /* libusb_get_device_descriptor */
};

enum irpc_usb_context {
    IRPC_USB_CONTEXT_CLIENT,        /* Client -> Server */
    IRPC_USB_CONTEXT_SERVER,        /* Server -> Client */
};

enum irpc_usb_retval {
    IRPC_RETVAL_FAILURE = -1,       /* Function call was success */
    IRPC_RETVAL_SUCCESS,            /* Function call has failed */
};

/* Holds connection specific information. */
struct irpc_usb_connection_info {
    int client_sock;                /* Client socked fd */
    int server_sock;                /* Server socket fd */
};

/* Reflection of libusb_device. */
struct irpc_usb_device {
    int bus_number;
    int device_address;
	int num_configurations;
    int session_data; // Just an identifier.
};

/* Reflection of libusb_device **. */
struct irpc_usb_device_list {
    int n_devs;
    struct irpc_usb_device devs[IRPC_MAX_DEVS];
};

/* Reflection of libusb_device_descriptor. */
struct irpc_usb_device_descriptor {
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

typedef enum irpc_usb_type irpc_usb_type;
typedef enum irpc_usb_context irpc_usb_context;
typedef enum irpc_usb_retval irpc_usb_retval;
typedef struct irpc_usb_connection_info irpc_usb_connection_info;
typedef struct irpc_usb_device_list irpc_usb_device_list;
typedef struct irpc_usb_device irpc_usb_device;
typedef struct irpc_usb_device_descriptor irpc_usb_device_descriptor;
