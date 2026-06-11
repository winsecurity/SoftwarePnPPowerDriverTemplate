#pragma once

#include <ntddk.h>
#include "irpdrainer.h"
#include "definitions.h"

#define TERMINATE_PROCESS CTL_CODE(FILE_DEVICE_UNKNOWN,0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct {
	UINT32 pid;
}PROCESS_ID, * PPROCESS_ID;



extern "C" NTSTATUS devicecontrolhandler(PDEVICE_OBJECT fdo, PIRP irp);


NTSTATUS TerminateProcess(UINT32 pid);

