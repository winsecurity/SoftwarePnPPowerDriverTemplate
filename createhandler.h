#pragma once

#include <ntddk.h>


extern "C" NTSTATUS createhandler(PDEVICE_OBJECT fdo, PIRP irp);
