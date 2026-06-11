#include "irphandler.h"


extern "C" NTSTATUS irphandler(PDEVICE_OBJECT fdo, PIRP irp) {


	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)fdo->DeviceExtension;

	IoAcquireRemoveLock(&pde->removelock, NULL);

	IoMarkIrpPending(irp);

	return QueueIRP(fdo, irp);

}



extern "C" void irpdispatcher(PDEVICE_OBJECT fdo, PIRP irp) {

	PIO_STACK_LOCATION iostacklocation = IoGetCurrentIrpStackLocation(irp);
	
	switch (iostacklocation->MajorFunction) {

	case IRP_MJ_READ:
		readhandler(fdo, irp);
		return;

	case IRP_MJ_DEVICE_CONTROL:
		devicecontrolhandler(fdo, irp);
		return;

	default:
		irp->IoStatus.Status = STATUS_CANCELLED;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return ;
	}


}

