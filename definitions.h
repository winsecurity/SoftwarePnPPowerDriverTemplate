#pragma once


typedef enum {
	STOPPED,
	WORKING,
	REMOVED
}DEVICESTATE;


typedef struct {
	KSPIN_LOCK spinlock;
	LIST_ENTRY irphead;
	PIO_WORKITEM workitem;	
	BOOLEAN acceptirps;
	volatile LONG workitemrunning;
	
}IRPQUEUE, *PIRPQUEUE;




typedef struct {
	PDEVICE_OBJECT lowerdeviceobject;
	IRPQUEUE irpqueue;
	volatile LONG iscriticaltaskrunning;
	volatile LONG isdevicebusy;
	IO_CSQ csq;
	KEVENT workitemrunningevent;
	IO_REMOVE_LOCK removelock;	
	UNICODE_STRING symboliclinkname;
	DEVICE_CAPABILITIES devicecapabilities;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;
