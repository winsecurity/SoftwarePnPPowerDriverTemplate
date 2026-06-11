#pragma once
#include <ntddk.h>
#include <wdmsec.h>


extern "C" NTSTATUS AddDevice(PDRIVER_OBJECT fdo, PDEVICE_OBJECT pdo);


