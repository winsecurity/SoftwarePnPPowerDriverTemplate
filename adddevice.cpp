#include "adddevice.h"
#include "definitions.h"
#include "csq.h"



extern "C" NTSTATUS AddDevice(PDRIVER_OBJECT driverobject, PDEVICE_OBJECT pdo) {

	UNICODE_STRING devicename;
	RtlInitUnicodeString(&devicename, L"\\Device\\softwarepnppowerdevice");

	// only admins and system users can open our device
	// D:P(A;;GA;;;SY)(A;;GA;;;BA)

	UNICODE_STRING sddlstring;
	RtlInitUnicodeString(&sddlstring, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");

	PDEVICE_OBJECT deviceobject;
	NTSTATUS res = IoCreateDeviceSecure(driverobject, sizeof(DEVICE_EXTENSION),
		&devicename, FILE_DEVICE_UNKNOWN, 0, FALSE, &sddlstring, NULL, &deviceobject);

	// Use normal IoCreateDevice if you dont want security 
	//NTSTATUS res = IoCreateDevice(driverobject, sizeof(DEVICE_EXTENSION),
	//	&devicename, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceobject);

	if (!NT_SUCCESS(res)) {
		KdPrint(("Device creation failed: %X\n", res));
		return res;
	}
		

	UNICODE_STRING symboliclink;
	RtlInitUnicodeString(&symboliclink, L"\\DosDevices\\softwarepnppowerdevicesymboliclink");
	
	res = IoCreateSymbolicLink(&symboliclink, &devicename);
	if (!NT_SUCCESS(res)) {
		KdPrint(("unable to create symbolic link: %X\n", res));
		IoDeleteDevice(deviceobject);
		return res;
	}


	// attaching our device to the devicestack

	PDEVICE_EXTENSION pde =  (PDEVICE_EXTENSION) deviceobject->DeviceExtension;

	pde->lowerdeviceobject = IoAttachDeviceToDeviceStack(deviceobject, pdo);

	if (pde->lowerdeviceobject == NULL) {
		KdPrint(("IoAttachDeviceToDeviceStack failed\n"));
		IoDeleteSymbolicLink(&symboliclink);
		IoDeleteDevice(deviceobject);
		return STATUS_UNSUCCESSFUL;
	}


	deviceobject->Flags |= DO_BUFFERED_IO;

	pde->symboliclinkname = symboliclink;
	pde->irpqueue.acceptirps = FALSE;
	KeInitializeSpinLock(&pde->irpqueue.spinlock);
	InitializeListHead(&pde->irpqueue.irphead);
	InterlockedExchange(&pde->iscriticaltaskrunning, 0);
	InterlockedExchange(&pde->isdevicebusy, 0);
	pde->irpqueue.workitem = IoAllocateWorkItem(deviceobject);
	if (pde->irpqueue.workitem == NULL) {
		// failed to allocate workitem
		IoDeleteSymbolicLink(&symboliclink);
		IoDeleteDevice(deviceobject);
		return STATUS_UNSUCCESSFUL;
	}

	pde->irpqueue.workitemrunning = FALSE;

	// setting workitemrunningevent to signaled, signaled means not working
	KeInitializeEvent(&pde->workitemrunningevent, NotificationEvent, TRUE);

	IoInitializeRemoveLock(&pde->removelock, NULL, 0, 0);

	res = IoCsqInitializeEx(&pde->csq, csqinsertirpex, csqremoveirp, csqpeeknextirp, csqacquirelock, csqreleaselock, csqcompletecanceledirp);
	if (!NT_SUCCESS(res)) {
		KdPrint(("failed to initialize csq: %X\n", res));
		IoDeleteSymbolicLink(&symboliclink);
		IoDeleteDevice(deviceobject);
		return res;	
	}




	return STATUS_SUCCESS;
		
}
