#pragma once
#include <ntddk.h>
#include "definitions.h"


extern "C" NTSTATUS pnphandler(PDEVICE_OBJECT fdo, PIRP irp);


NTSTATUS startdevicecompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context);


NTSTATUS querystopdevicecompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context);
NTSTATUS queryremovedevicecompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context);

NTSTATUS removedevicecompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context);


NTSTATUS querycapabilitiescompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context);


