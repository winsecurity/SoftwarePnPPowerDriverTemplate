#include "adddevice.h"







extern "C" NTSTATUS AddDevice(PDRIVER_OBJECT driverobject, PDEVICE_OBJECT pdo) {

	KdPrint(("Inside AddDevice\n"));

	//UNICODE_STRING devicename;
	//RtlInitUnicodeString(&devicename, L"\\Device\\softwarepnppowerdevice");

	// only admins and system users can open our device
	// D:P(A;;GA;;;SY)(A;;GA;;;BA)

	UNICODE_STRING sddlstring;
	RtlInitUnicodeString(&sddlstring, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");

	PDEVICE_OBJECT deviceobject;
	//NTSTATUS res = IoCreateDeviceSecure(driverobject, sizeof(DEVICE_EXTENSION),
	//	NULL, FILE_DEVICE_UNKNOWN, 0, FALSE, &sddlstring, &GUID_DEVCLASS_MYDEVICE2, &deviceobject);

	// Use normal IoCreateDevice if you dont want security 
	NTSTATUS res = IoCreateDevice(driverobject, sizeof(DEVICE_EXTENSION),
		NULL, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceobject);

	if (!NT_SUCCESS(res)) {
		KdPrint(("creating device secure failed: %X\n", res));
		return res;
	}


	/*UNICODE_STRING symboliclink;
	RtlInitUnicodeString(&symboliclink, L"\\DosDevices\\softwarepnppowerdevicesymboliclink");
	
	res = IoCreateSymbolicLink(&symboliclink, &devicename);
	if (!NT_SUCCESS(res)) {
		KdPrint(("unable to create symbolic link: %X\n", res));
		IoDeleteDevice(deviceobject);
		return res;
	}*/

	
	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)deviceobject->DeviceExtension;
	RtlZeroMemory(pde, sizeof(DEVICE_EXTENSION));


	// attaching our device to the devicestack

	KdPrint(("Attaching our device to device stack\n"));
	pde->lowerdeviceobject = IoAttachDeviceToDeviceStack(deviceobject, pdo);

	if (pde->lowerdeviceobject == NULL) {
		KdPrint(("IoAttachDeviceToDeviceStack failed\n"));
		//IoDeleteSymbolicLink(&symboliclink);
		IoDeleteDevice(deviceobject);
		return STATUS_UNSUCCESSFUL;
	}


	KdPrint(("successfully attached\n"));

	KdPrint(("Registering device interface\n"));
	res = IoRegisterDeviceInterface(pdo, &GUID_DEVINTERFACE_MYDEVICE2, NULL, &pde->symboliclinkname);
	if (res == STATUS_OBJECT_NAME_EXISTS) {
		KdPrint(("Device interface already exists\n"));
		KdPrint(("Symboliclinkname: %wZ\n", pde->symboliclinkname));
	}
	else if (NT_SUCCESS(res)) {
		KdPrint(("Device interface registered successfully\n"));
		KdPrint(("Symboliclinkname: %wZ\n", pde->symboliclinkname));
	}
	else {
		KdPrint(("Device registration failed: %X\n", res));
		IoDetachDevice(pde->lowerdeviceobject);
		IoDeleteDevice(deviceobject);
		return res;
	}


	KdPrint(("Device is initializing\n"));
	
	deviceobject->Flags |= DO_BUFFERED_IO;


	pde->isdeviceinterfaceenabled = FALSE;

	pde->irpqueue.acceptirps = FALSE;
	KeInitializeSpinLock(&pde->irpqueue.spinlock);
	InitializeListHead(&pde->irpqueue.irphead);
	InterlockedExchange(&pde->iscriticaltaskrunning, 0);
	InterlockedExchange(&pde->isdevicebusy, 0);
	pde->irpqueue.workitem = IoAllocateWorkItem(deviceobject);
	if (pde->irpqueue.workitem == NULL) {
		// failed to allocate workitem
		//IoDeleteSymbolicLink(&symboliclink);
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
		//IoDeleteSymbolicLink(&symboliclink);
		IoDeleteDevice(deviceobject);
		return res;	
	}


	deviceobject->Flags &= ~DO_DEVICE_INITIALIZING;

	KdPrint(("Device initialization done	\n"));

	return STATUS_SUCCESS;
		
}
