#include "csq.h"



void csqacquirelock(PIO_CSQ csq, PKIRQL irql) {

	PDEVICE_EXTENSION pde = CONTAINING_RECORD(csq, DEVICE_EXTENSION, csq);
	KeAcquireSpinLock(&pde->irpqueue.spinlock, irql);

}

	
void csqreleaselock(PIO_CSQ csq, KIRQL irql) {
	PDEVICE_EXTENSION pde = CONTAINING_RECORD(csq, DEVICE_EXTENSION, csq);
	KeReleaseSpinLock(&pde->irpqueue.spinlock, irql);

}


void csqinsertirp(PIO_CSQ csq, PIRP irp) {

	PDEVICE_EXTENSION pde = CONTAINING_RECORD(csq, DEVICE_EXTENSION, csq);


	if (pde->irpqueue.acceptirps == FALSE) {
		// we cannot accept irps, cancelling them
		irp->IoStatus.Status = STATUS_CANCELLED;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		IoReleaseRemoveLock(&pde->removelock, NULL);
		return;	
	}


	InsertTailList(&pde->irpqueue.irphead, &irp->Tail.Overlay.ListEntry);

	// compare workitemrunning to 0, if it's 0 then we put 1 value indicating
	// workitemrunning is running and reset event to nonsignaled state and queue workitem
	if (InterlockedCompareExchange(&pde->irpqueue.workitemrunning,1,0)== 0) {
	
		// resetting workitemrunningevent to non-signaled state
		KeResetEvent(&pde->workitemrunningevent);
		IoQueueWorkItem(pde->irpqueue.workitem, workitemroutine, DelayedWorkQueue, NULL);

	}

}




NTSTATUS csqinsertirpex(PIO_CSQ csq, PIRP irp, PVOID insertcontext) {

	PDEVICE_EXTENSION pde = CONTAINING_RECORD(csq, DEVICE_EXTENSION, csq);


	if (pde->irpqueue.acceptirps == FALSE) {
		// we cannot accept irps, cancelling them
		return STATUS_CANCELLED;
	}


	KdPrint(("Inserting IRP: %p\n", irp));
	InsertTailList(&pde->irpqueue.irphead, &irp->Tail.Overlay.ListEntry);

	// compare workitemrunning to 0, if it's 0 then we put 1 value indicating
	// workitemrunning is running and reset event to nonsignaled state and queue workitem
	if (InterlockedCompareExchange(&pde->irpqueue.workitemrunning,1,0) == 0) {

		// resetting workitemrunningevent to non-signaled state, means workitem is running
		KeResetEvent(&pde->workitemrunningevent);
		
		//InterlockedExchange(&pde->irpqueue.workitemrunning, 1);

		*(bool*)insertcontext = TRUE;

		//IoQueueWorkItem(pde->irpqueue.workitem, workitemroutine, DelayedWorkQueue, NULL);

	}


	return STATUS_SUCCESS;

}



	
void csqremoveirp(PIO_CSQ csq, PIRP irp) {
	UNREFERENCED_PARAMETER(csq);
	//PDEVICE_EXTENSION pde = CONTAINING_RECORD(csq, DEVICE_EXTENSION, csq);

	RemoveEntryList(&irp->Tail.Overlay.ListEntry);


}




void csqcompletecanceledirp(PIO_CSQ csq, PIRP irp) {


	PDEVICE_EXTENSION pde = CONTAINING_RECORD(csq, DEVICE_EXTENSION, csq);

	irp->IoStatus.Status = STATUS_CANCELLED;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	IoReleaseRemoveLock(&pde->removelock, NULL);

}



PIRP csqpeeknextirp(PIO_CSQ csq, PIRP irp, PVOID peekcontext) {
	UNREFERENCED_PARAMETER(peekcontext);
	PDEVICE_EXTENSION pde = CONTAINING_RECORD(csq, DEVICE_EXTENSION, csq);

	PLIST_ENTRY le;
	if (irp == NULL) {
		// return first item
		if (IsListEmpty(&pde->irpqueue.irphead)) {
			// no items
			return NULL;
		}
		le = pde->irpqueue.irphead.Flink;
		return CONTAINING_RECORD(le, IRP, Tail.Overlay.ListEntry);

	}


	le = irp->Tail.Overlay.ListEntry.Flink;
	if (le == &pde->irpqueue.irphead) {
		// this is the last irp, return null
		return NULL;
	}

	// else return irp next to the desired irp
	return CONTAINING_RECORD(le, IRP, Tail.Overlay.ListEntry);



}
