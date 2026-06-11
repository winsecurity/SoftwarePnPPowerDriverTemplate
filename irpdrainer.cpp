#include "irpdrainer.h"



void cancelroutine(PDEVICE_OBJECT fdo, PIRP irp) {

	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	

	// removing the irp from the queue
	KIRQL oldirql;
	KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);
	IoReleaseCancelSpinLock(irp->CancelIrql);
	RemoveEntryList(&irp->Tail.Overlay.ListEntry);
	KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);


	// cancelling the irp
	irp->IoStatus.Status = STATUS_CANCELLED;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);



}



/*void workitemroutine(PDEVICE_OBJECT fdo, PVOID context) {

	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	
	KIRQL oldirql;
	KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);

	while (!IsListEmpty(&pde->irpqueue.irphead)) {

		// fetch first irp
		KIRQL oldirql2;
		IoAcquireCancelSpinLock(&oldirql2);
		PLIST_ENTRY le = pde->irpqueue.irphead.Flink;
		PIRP irp = CONTAINING_RECORD(le, IRP, Tail.Overlay.ListEntry);

		PDRIVER_CANCEL previouscancelroutine = IoSetCancelRoutine(irp, NULL);

		if (previouscancelroutine == NULL) {
			// if previous cancel routine is NULL
			// then irp is being cancelled or scheduled to cancel
			// we should not touch the irp, cancel routine deals with it
			IoReleaseCancelSpinLock(oldirql2);
			continue;
		}
	

		// we now own the irp and remove it from the queue
		RemoveEntryList(&irp->Tail.Overlay.ListEntry);
		IoReleaseCancelSpinLock(oldirql2);
		KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);
		
		if (irp->Cancel) {
			irp->IoStatus.Status = STATUS_CANCELLED;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);
			continue;
		}
		

		// else we need to complete the irp
		// handle irp


		KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);

	}
	
	KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);

}

*/


void workitemroutine(PDEVICE_OBJECT fdo, PVOID context) {
	UNREFERENCED_PARAMETER(context);
	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)fdo->DeviceExtension;

	while (TRUE) {

		while (PIRP irp = IoCsqRemoveNextIrp(&pde->csq, NULL)) {

			KdPrint(("Processing IRP: %p\n", irp));

			if (irp->Cancel) {
				KdPrint(("Cancelling IRP: %p inside workitem\n"));
				irp->IoStatus.Status = STATUS_CANCELLED;
				irp->IoStatus.Information = 0;
				IoCompleteRequest(irp, IO_NO_INCREMENT);
				IoReleaseRemoveLock(&pde->removelock, NULL);
				continue;
			}


			// handle irp
			irpdispatcher(fdo, irp);



			IoReleaseRemoveLock(&pde->removelock, NULL);

		}

		KIRQL oldirql;
		KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);

		if (IsListEmpty(&pde->irpqueue.irphead)) {
			KdPrint(("IRP queue empty, exiting workitem\n"));
			InterlockedExchange(&pde->irpqueue.workitemrunning, 0);
			// setting workitemrunningevent to signaled
			KeSetEvent(&pde->workitemrunningevent, IO_NO_INCREMENT, FALSE);
			KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);
			return;
		}

		KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);

	}

	


}


/*void QueueIRP(PDEVICE_OBJECT fdo, PIRP irp) {

	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)fdo->DeviceExtension;



	// Acquiring the spinlock of the irpqueue
	KIRQL oldirql;
	KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);

	if (pde->irpqueue.acceptirps == FALSE) {
		// our device cannot accept irps right now
		// cancelling the arrived irp
		KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);
		irp->IoStatus.Status = STATUS_DEVICE_BUSY;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return;
	}

	
	
	KIRQL oldirql2;
	IoAcquireCancelSpinLock(&oldirql2);

	if (irp->Cancel) {
		// cancel bit is set, irp must cancel
		// since we did not set cancel routine, we need to manually cancel
		irp->IoStatus.Status = STATUS_DEVICE_BUSY;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		IoReleaseCancelSpinLock(oldirql2);
		KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);
		return;
	}
	

	// if cancel bit is not set then we need to set cancel routine

	PDRIVER_CANCEL previouscancelroutine = IoSetCancelRoutine(irp, cancelroutine);

	IoReleaseCancelSpinLock(oldirql2);


	BOOLEAN islistempty = IsListEmpty(&pde->irpqueue.irphead);

	InsertTailList(&pde->irpqueue.irphead, &irp->Tail.Overlay.ListEntry);

	if (islistempty) {
		// no workitem running, so queue
		pde->irpqueue.workitemrunning = TRUE;
		IoQueueWorkItem(pde->irpqueue.workitem, workitemroutine, DelayedWorkQueue, NULL);
		
	}

	KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);



}
*/

NTSTATUS QueueIRP(PDEVICE_OBJECT fdo, PIRP irp) {

	
	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)fdo->DeviceExtension;

	bool shouldqueueworkitem = FALSE;
	NTSTATUS res = IoCsqInsertIrpEx(&pde->csq, irp, NULL, &shouldqueueworkitem);
	if (!NT_SUCCESS(res)) {
		KdPrint(("Inserting IRP into CSQ failed: %X\n", res));
		irp->IoStatus.Status = res;
		IoReleaseRemoveLock(&pde->removelock, NULL);
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return res;
	}

	// irp has been inserted successfully
	// if we need to queueworkitem then we queue
	if (shouldqueueworkitem) {
		KdPrint(("Queueing workitem\n"));
		IoQueueWorkItem(pde->irpqueue.workitem, workitemroutine, DelayedWorkQueue, NULL);
	}


	return STATUS_PENDING;

}

