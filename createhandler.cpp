#include "createhandler.h"

extern "C" NTSTATUS createhandler(PDEVICE_OBJECT , PIRP irp) {

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;

}
