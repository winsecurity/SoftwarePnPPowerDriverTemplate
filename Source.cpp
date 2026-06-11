#include <ntddk.h>

#include "adddevice.h"
#include "createhandler.h"


#include "pnphandler.h"
#include "powerhandler.h"
#include "irphandler.h"

extern "C" void DriverUnload(PDRIVER_OBJECT driverobject) {
	UNREFERENCED_PARAMETER(driverobject);
	/*PDEVICE_OBJECT firstdevice = driverobject->DeviceObject;
	
	while (firstdevice != NULL) {

		PDEVICE_OBJECT temp = firstdevice->NextDevice;
		IoDeleteDevice(firstdevice);
		firstdevice = temp->NextDevice;

	}*/
	

}



extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driverobject, PUNICODE_STRING registrypath) {

	UNREFERENCED_PARAMETER(registrypath);


	driverobject->DriverUnload = DriverUnload;
	driverobject->DriverExtension->AddDevice = AddDevice;

	driverobject->MajorFunction[IRP_MJ_CREATE] = createhandler;
	driverobject->MajorFunction[IRP_MJ_CLOSE] = createhandler;

	driverobject->MajorFunction[IRP_MJ_READ] = irphandler;
	driverobject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = irphandler;

	driverobject->MajorFunction[IRP_MJ_PNP] = pnphandler;
	driverobject->MajorFunction[IRP_MJ_POWER] = powerhandler;

	return STATUS_SUCCESS;

}
