#pragma once

#include <ntifs.h>
#include "definitions.h"
#include "irpdrainer.h"

extern "C" NTSTATUS readhandler(PDEVICE_OBJECT fdo, PIRP irp);

