#pragma once
#include <ntifs.h>
#include <wdmsec.h>
#include "definitions.h"
#include "csq.h"

#include "initguid.h"
#include "wdmguid.h"


// {61D718A0-22AE-43D6-AFB0-A71B9B2D571C}
DEFINE_GUID(GUID_DEVCLASS_MYDEVICE2 ,
	0x61d718a0, 0x22ae, 0x43d6, 0xaf, 0xb0, 0xa7, 0x1b, 0x9b, 0x2d, 0x57, 0x1c);



// {4517B782-E4C7-4934-ABCE-4C878FDCE6CE}
DEFINE_GUID(GUID_DEVINTERFACE_MYDEVICE2, 0x4517b782, 0xe4c7, 
	0x4934, 0xab, 0xce, 0x4c, 0x87, 0x8f, 0xdc, 0xe6, 0xce);


extern "C" NTSTATUS AddDevice(PDRIVER_OBJECT fdo, PDEVICE_OBJECT pdo);


