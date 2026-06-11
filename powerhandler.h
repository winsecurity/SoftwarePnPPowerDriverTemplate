#pragma once
#include <ntddk.h>
#include "definitions.h"


extern "C" NTSTATUS powerhandler(PDEVICE_OBJECT fdo, PIRP irp);


NTSTATUS querypowercompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context);

void querypowercallback(PDEVICE_OBJECT deviceobject, UCHAR minorfunction, POWER_STATE powerstate, PVOID context, PIO_STATUS_BLOCK isb);



NTSTATUS setpowercompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context);

void setpowercallback(PDEVICE_OBJECT deviceobject, UCHAR minorfunction, POWER_STATE powerstate, PVOID context, PIO_STATUS_BLOCK isb);
