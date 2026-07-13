#pragma once

#include <ntifs.h>


extern "C" NTSTATUS createhandler(PDEVICE_OBJECT fdo, PIRP irp);
