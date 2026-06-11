#include "pnphandler.h"

NTSTATUS startdevicecompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context) {
	UNREFERENCED_PARAMETER(deviceobject);
	UNREFERENCED_PARAMETER(irp);
	KEVENT* waitevent = (KEVENT*)context;

	//KdPrint(("signalling the event\n"));

	KeSetEvent(waitevent, IO_NO_INCREMENT, FALSE);


	return STATUS_MORE_PROCESSING_REQUIRED;


}


NTSTATUS querystopdevicecompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context) {
	UNREFERENCED_PARAMETER(deviceobject);
	UNREFERENCED_PARAMETER(irp);
	KEVENT* waitevent = (KEVENT*)context;

	KeSetEvent(waitevent, IO_NO_INCREMENT, FALSE);

	return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS queryremovedevicecompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context) {
	UNREFERENCED_PARAMETER(deviceobject);
	UNREFERENCED_PARAMETER(irp);
	KEVENT* waitevent = (KEVENT*)context;

	KeSetEvent(waitevent, IO_NO_INCREMENT, FALSE);

	return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS removedevicecompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context) {
	UNREFERENCED_PARAMETER(deviceobject);
	UNREFERENCED_PARAMETER(irp);
	KEVENT* waitevent = (KEVENT*)context;

	KeSetEvent(waitevent, IO_NO_INCREMENT, FALSE);

	return STATUS_MORE_PROCESSING_REQUIRED;

}




NTSTATUS querycapabilitiescompletionroutine(PDEVICE_OBJECT deviceobject, PIRP irp, PVOID context) {

	UNREFERENCED_PARAMETER(deviceobject);
	PDEVICE_OBJECT fdo = (PDEVICE_OBJECT) context;
	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION iostacklocation = IoGetCurrentIrpStackLocation(irp);


	PDEVICE_CAPABILITIES pdevicecapabilities = iostacklocation->Parameters.DeviceCapabilities.Capabilities;

	pdevicecapabilities->Removable = TRUE;
	pdevicecapabilities->SurpriseRemovalOK = TRUE;

	// setting system-device power mapping states
	pdevicecapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
	pdevicecapabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
	pdevicecapabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
	pdevicecapabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
	pdevicecapabilities->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
	pdevicecapabilities->DeviceState[PowerSystemShutdown] = PowerDeviceD3;
	
	pdevicecapabilities->SystemWake = PowerSystemUnspecified;
	pdevicecapabilities->DeviceWake = PowerDeviceUnspecified;

	RtlCopyMemory(&pde->devicecapabilities, pdevicecapabilities, sizeof(DEVICE_CAPABILITIES));


	return STATUS_CONTINUE_COMPLETION;

}



extern "C" NTSTATUS pnphandler(PDEVICE_OBJECT fdo, PIRP irp) {

	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION iostacklocation = IoGetCurrentIrpStackLocation(irp);




	if (iostacklocation->MinorFunction == IRP_MN_QUERY_CAPABILITIES) {

		KdPrint(("we got IRP_MN_QUERY_CAPABILITIES\n"));

		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutineEx(fdo, irp, querycapabilitiescompletionroutine, fdo, TRUE, TRUE, TRUE);
		return IoCallDriver(pde->lowerdeviceobject, irp);

		
	}



	if (iostacklocation->MinorFunction == IRP_MN_START_DEVICE) {

		KdPrint(("we got IRP_MN_START_DEVICE\n"));

		KEVENT waitevent;
		KeInitializeEvent(&waitevent, NotificationEvent, FALSE);
		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, startdevicecompletionroutine, &waitevent, TRUE, TRUE, TRUE);

		IoCallDriver(pde->lowerdeviceobject, irp);

		// waiting for lower devices to complete the irp
		KeWaitForSingleObject(&waitevent, Executive, KernelMode, FALSE, NULL);

		NTSTATUS res = irp->IoStatus.Status;
		if (NT_SUCCESS(res)) {
			
			KdPrint(("lower drivers succeeded start_Device\n"));

			fdo->Flags &= ~DO_DEVICE_INITIALIZING;

			pde->irpqueue.acceptirps = TRUE;


			irp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_SUCCESS;


		}

		else {

			KdPrint(("start device failed by lower devices\n"));

			pde->irpqueue.acceptirps = FALSE;

			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return res;

		}



	}



	if (iostacklocation->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) {

		KdPrint(("we got IRP_MN_QUERY_STOP_DEVICE\n"));

		if (InterlockedCompareExchange(&pde->iscriticaltaskrunning,0,0) == 1) {
			// critical taks running, we can fail this query stop device irp
			KdPrint(("Critical tasks running, failing IRP_MN_QUERY_STOP_DEVICE\n"));
			irp->IoStatus.Status = STATUS_DEVICE_BUSY;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_DEVICE_BUSY;
		}

		// gating irps with acceptirps
		KIRQL oldirql;
		KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);
		pde->irpqueue.acceptirps = FALSE;
		KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);

		// draining irps
		// waiting for workitem to complete the irps
		
		KeWaitForSingleObject(&pde->workitemrunningevent, Executive, KernelMode, FALSE, NULL);


		// else set status to success and pass down
		// check if all lower drivers succeeded the irp
		// if not then rollback acceptirps statew		
		irp->IoStatus.Status = STATUS_SUCCESS;
		KEVENT event;
		KeInitializeEvent(&event, NotificationEvent, FALSE);	
		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, querystopdevicecompletionroutine, &event, TRUE, TRUE, TRUE);
		NTSTATUS res= IoCallDriver(pde->lowerdeviceobject, irp);

		if (res == STATUS_PENDING) {
			KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
			
		}
		res = irp->IoStatus.Status;	
		if (!NT_SUCCESS(res)) {
			//rollback
			KIRQL oldirql2;
			KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql2);
			pde->irpqueue.acceptirps = TRUE;
			KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql2);
		}
		
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return res;


	}


	if (iostacklocation->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE) {


		KdPrint(("we got IRP_MN_CANCEL_STOP_DEVICE\n"));

		// stopdevice is not coming, we can start accepting irps
		KIRQL oldirql;
		KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);
		pde->irpqueue.acceptirps = TRUE;
		KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);

		irp->IoStatus.Status = STATUS_SUCCESS;
		IoSkipCurrentIrpStackLocation(irp);
		return IoCallDriver(pde->lowerdeviceobject, irp);


	}


	if (iostacklocation->MinorFunction == IRP_MN_STOP_DEVICE) {
		
		KdPrint(("we got IRP_MN_STOP_DEVICE\n"));

		// this is a command
		// wait until  removelock is released completely
		// meaning all irps are drained
		IoReleaseRemoveLockAndWait(&pde->removelock, NULL);
		

		irp->IoStatus.Status = STATUS_SUCCESS;
		IoSkipCurrentIrpStackLocation(irp);
		return IoCallDriver(pde->lowerdeviceobject, irp);


	}


	if (iostacklocation->MinorFunction == IRP_MN_SURPRISE_REMOVAL) {
	
		KdPrint(("we got IRP_MN_SURPRISE_REMOVAL\n"));

		KIRQL oldirql;
		KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);
		pde->irpqueue.acceptirps = FALSE;
		KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);
	
		irp->IoStatus.Status = STATUS_SUCCESS;
		IoSkipCurrentIrpStackLocation(irp);
		return IoCallDriver(pde->lowerdeviceobject, irp);

	}




	if (iostacklocation->MinorFunction == IRP_MN_QUERY_REMOVE_DEVICE) {
		KdPrint(("we got IRP_MN_QUERY_REMOVE_DEVICE\n"));

		// should implement completion routine and stop accepting irps only if irp succeeds
		if (InterlockedCompareExchange(&pde->iscriticaltaskrunning, 0, 0) == 1) {
			// criticaltask is running, we cant get removed
			irp->IoStatus.Status = STATUS_DEVICE_BUSY;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_DEVICE_BUSY;
		}

		KIRQL oldirql;
		KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);
		pde->irpqueue.acceptirps = FALSE;
		KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);

		// draining irps
		// waiting for workitem to complete the irps
		KeWaitForSingleObject(&pde->workitemrunningevent, Executive, KernelMode, FALSE, NULL);
		


		// else set status to success and pass down
		// check if all lower drivers succeeded the irp
		// if not then rollback acceptirps statew		
		irp->IoStatus.Status = STATUS_SUCCESS;
		KEVENT event;
		KeInitializeEvent(&event, NotificationEvent, FALSE);
		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, queryremovedevicecompletionroutine, &event, TRUE, TRUE, TRUE);
		NTSTATUS res = IoCallDriver(pde->lowerdeviceobject, irp);

		if (res == STATUS_PENDING) {
			KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

		}
		res = irp->IoStatus.Status;
		if (!NT_SUCCESS(res)) {
			//rollback
			KIRQL oldirql2;
			KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql2);
			pde->irpqueue.acceptirps = TRUE;
			KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql2);
		}

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return res;


	}



	if (iostacklocation->MinorFunction == IRP_MN_CANCEL_REMOVE_DEVICE) {
		KdPrint(("we got IRP_MN_CANCEL_REMOVE_DEVICE\n"));

		KIRQL oldirql;
		KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);
		pde->irpqueue.acceptirps = TRUE;
		KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);

		irp->IoStatus.Status = STATUS_SUCCESS;
		IoSkipCurrentIrpStackLocation(irp);
		return IoCallDriver(pde->lowerdeviceobject, irp);


	}




	if (iostacklocation->MinorFunction ==	IRP_MN_REMOVE_DEVICE) {

		KdPrint(("we got IRP_MN_REMOVE_DEVICE\n"));

		// this is a command
		// we need to packup, stop accepting irps, drain irps and remove deviceobject

		KIRQL oldirql;
		KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);
		pde->irpqueue.acceptirps = FALSE;
		KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);


		// wait for workitem to finish
		if (InterlockedCompareExchange(&pde->irpqueue.workitemrunning, 1, 1) == 1) {
			KdPrint(("workitem running, waiting for workitemrunningevent\n"));
			KeWaitForSingleObject(&pde->workitemrunningevent, Executive, KernelMode, FALSE, NULL);

		}
		

		// cancelling remaining irps
		while (PIRP cancelirp = IoCsqRemoveNextIrp(&pde->csq, NULL)) {

			// cancelling the remaining irps
			cancelirp->IoStatus.Status = STATUS_DEVICE_REMOVED;
			cancelirp->IoStatus.Information = 0;
			IoCompleteRequest(cancelirp, IO_NO_INCREMENT);
			IoReleaseRemoveLock(&pde->removelock, NULL);
		}


		
		IoDeleteSymbolicLink(&pde->symboliclinkname);


		KEVENT event;
		KeInitializeEvent(&event, NotificationEvent, FALSE);
		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, removedevicecompletionroutine, &event, TRUE, TRUE, TRUE);
		NTSTATUS res = IoCallDriver(pde->lowerdeviceobject, irp);

		KdPrint(("waiting for remove device irp to complete by lwoer drivers\n"));
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

		// irp has been passed down 
		// now we can remove our devices

		// wait for removelocks
		IoAcquireRemoveLock(&pde->removelock, NULL);
		KdPrint(("waiting for all removelocks to get released\n"));
		IoReleaseRemoveLockAndWait(&pde->removelock, NULL);


		IoFreeWorkItem(pde->irpqueue.workitem);

		IoDetachDevice(pde->lowerdeviceobject);
		IoDeleteDevice(fdo);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return res;
	
		


	}



	IoSkipCurrentIrpStackLocation(irp);
	return IoCallDriver(pde->lowerdeviceobject, irp);


}
