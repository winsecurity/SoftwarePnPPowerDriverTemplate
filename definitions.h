#pragma once



typedef struct _EX_FAST_REF
{
	union
	{
		PVOID Object;
		ULONG RefCnt : 3;
		ULONG Value;
	};
} EX_FAST_REF, * PEX_FAST_REF;


typedef struct _CONTROL_AREA
{
	ULONG_PTR Segment;
	LIST_ENTRY DereferenceList;
	ULONG NumberOfSectionReferences;
	ULONG NumberOfPfnReferences;
	ULONG NumberOfMappedViews;
	ULONG NumberOfUserReferences;
	ULONG u;
	ULONG u1;
	EX_FAST_REF FilePointer;
	LONG ControlAreaLock;
	ULONG StartingFrame;
	ULONG_PTR WaitingForDeletion;
	char u2[12];
	INT64 LockedPages;
} CONTROL_AREA, * PCONTROL_AREA;





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
	
	DEVICE_CAPABILITIES devicecapabilities;
	UNICODE_STRING symboliclinkname;
	BOOLEAN isdeviceinterfaceenabled;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;
