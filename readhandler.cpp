#include "readhandler.h"



extern "C" NTSTATUS readhandler(PDEVICE_OBJECT fdo, PIRP irp) {

	UNREFERENCED_PARAMETER(fdo);

	KdPrint(("we got IRP_MJ_READ\n"));

	PIO_STACK_LOCATION iostacklocation = IoGetCurrentIrpStackLocation(irp);
		
	if ((fdo->Flags & DO_BUFFERED_IO) == DO_BUFFERED_IO) {

		ULONG bufferlength = iostacklocation->Parameters.Read.Length;

		PVOID systembuffer = irp->AssociatedIrp.SystemBuffer;

		KdPrint(("systembuffer at: %p\n", systembuffer));
		KdPrint(("user buffer: %p\n", irp->UserBuffer));

		const char* msg = "Hello there from driver\0";
		if (strlen(msg) + 1 > bufferlength) {
			irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_BUFFER_TOO_SMALL;
		}


		RtlCopyMemory(systembuffer, msg, strlen(msg) + 1);

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = strlen(msg) + 1;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;


	}



	irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_UNSUCCESSFUL;


}

