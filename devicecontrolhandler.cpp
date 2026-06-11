
#include "devicecontrolhandler.h"


NTSTATUS TerminateProcess(UINT32 pid) {

	OBJECT_ATTRIBUTES oa;
	InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
	CLIENT_ID clientid{ 0 };
	clientid.UniqueProcess = (HANDLE)pid;

	HANDLE processhandle;
	NTSTATUS res = ZwOpenProcess(&processhandle, PROCESS_ALL_ACCESS, &oa, &clientid);
	if (!NT_SUCCESS(res)) {
		return res;
	}


	res = ZwTerminateProcess(processhandle, STATUS_PROCESS_IS_TERMINATING);
	if (!NT_SUCCESS(res)) {
		ZwClose(processhandle);
		return res;
	}


	ZwClose(processhandle);
	return STATUS_SUCCESS;



}



// we no need to return here since our irphandler returned QueueIRP() already
extern "C" NTSTATUS devicecontrolhandler(PDEVICE_OBJECT fdo, PIRP irp) {

	UNREFERENCED_PARAMETER(fdo);

	KdPrint(("we got IRP_MJ_DEVICE_CONTROL\n"));

	PIO_STACK_LOCATION iostacklocation = IoGetCurrentIrpStackLocation(irp);
	ULONG ioctl = iostacklocation->Parameters.DeviceIoControl.IoControlCode;



	if ((fdo->Flags & DO_BUFFERED_IO) == DO_BUFFERED_IO) {

		PVOID systembuffer = irp->AssociatedIrp.SystemBuffer;


		if (ioctl == TERMINATE_PROCESS) {

			ULONG inputbufferlength = iostacklocation->Parameters.DeviceIoControl.InputBufferLength;
			//ULONG outputbufferlength = iostacklocation->Parameters.DeviceIoControl.OutputBufferLength;


			if (inputbufferlength != sizeof(PROCESS_ID)) {
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				irp->IoStatus.Information = 0;
				IoCompleteRequest(irp, IO_NO_INCREMENT);
				return STATUS_BUFFER_TOO_SMALL;
			}

			PPROCESS_ID pid = (PPROCESS_ID)systembuffer;
			NTSTATUS res = TerminateProcess(pid->pid);
			irp->IoStatus.Status = res;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return res;




		}






	}

	
	irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_UNSUCCESSFUL;	

}
