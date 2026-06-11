#pragma once
#include <ntddk.h>

#include "definitions.h"
#include "irpdrainer.h"
#include "readhandler.h"
#include "devicecontrolhandler.h"


extern "C" NTSTATUS irphandler(PDEVICE_OBJECT fdo, PIRP irp);

extern "C" void irpdispatcher(PDEVICE_OBJECT fdo, PIRP irp);

