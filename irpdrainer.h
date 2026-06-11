#pragma once
#include <ntddk.h>

#include "irphandler.h"
#include "definitions.h"


NTSTATUS QueueIRP(PDEVICE_OBJECT fdo, PIRP irp);

void cancelroutine(PDEVICE_OBJECT fdo, PIRP irp);

void workitemroutine(PDEVICE_OBJECT fdo, PVOID context);

	
