#include "powerhandler.h"




void querypowercallback(PDEVICE_OBJECT deviceobject, UCHAR minorfunction, 
	POWER_STATE powerstate, PVOID context, PIO_STATUS_BLOCK isb) {
	UNREFERENCED_PARAMETER(powerstate);
	UNREFERENCED_PARAMETER(minorfunction);
	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)deviceobject->DeviceExtension;

	PIRP systemirp = (PIRP)context;

		
	systemirp->IoStatus.Status = isb->Status;
	systemirp->IoStatus.Information = isb->Information;
	IoCompleteRequest(systemirp, IO_NO_INCREMENT);


	IoReleaseRemoveLock(&pde->removelock, NULL);

}



NTSTATUS querypowercompletionroutine(PDEVICE_OBJECT , PIRP irp, PVOID context) {


	PDEVICE_OBJECT fdo = (PDEVICE_OBJECT)context;
	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION iostacklocation = IoGetCurrentIrpStackLocation(irp);

	if (!NT_SUCCESS(irp->IoStatus.Status)) {
		// if the systempowerstate irp failed then we just return
		IoReleaseRemoveLock(&pde->removelock, NULL);
		return STATUS_CONTINUE_COMPLETION;
	}

	if (irp->PendingReturned) {
		IoMarkIrpPending(irp);
	}

	POWER_STATE ps;
	ps.DeviceState = pde->devicecapabilities.DeviceState[iostacklocation->Parameters.Power.State.SystemState];
	NTSTATUS res = PoRequestPowerIrp(fdo, IRP_MN_QUERY_POWER, ps, querypowercallback, irp, NULL);

	if (res == STATUS_PENDING) {
		// then device state power irp has been sent successfully
		return STATUS_MORE_PROCESSING_REQUIRED;
	}


	IoReleaseRemoveLock(&pde->removelock, NULL);
	irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
	return STATUS_INSUFFICIENT_RESOURCES;


}




void setpowercallback(PDEVICE_OBJECT deviceobject, UCHAR minorfunction,
	POWER_STATE powerstate, PVOID context, PIO_STATUS_BLOCK isb) {

	UNREFERENCED_PARAMETER(powerstate);
	UNREFERENCED_PARAMETER(minorfunction);

	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)deviceobject->DeviceExtension;

	PIRP systemirp = (PIRP)context;

	systemirp->IoStatus.Status = isb->Status;
	systemirp->IoStatus.Information = isb->Information;
	IoCompleteRequest(systemirp, IO_NO_INCREMENT);


	IoReleaseRemoveLock(&pde->removelock, NULL);

}



NTSTATUS setpowercompletionroutine(PDEVICE_OBJECT , PIRP irp, PVOID context) {


	PDEVICE_OBJECT fdo = (PDEVICE_OBJECT)context;
	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION iostacklocation = IoGetCurrentIrpStackLocation(irp);

	if (!NT_SUCCESS(irp->IoStatus.Status)) {
		// if the systempowerstate irp failed then we just return
		IoReleaseRemoveLock(&pde->removelock, NULL);
		return STATUS_CONTINUE_COMPLETION;
	}

	if (irp->PendingReturned) {
		IoMarkIrpPending(irp);
	}

	POWER_STATE ps;
	ps.DeviceState = pde->devicecapabilities.DeviceState[iostacklocation->Parameters.Power.State.SystemState];
	NTSTATUS res = PoRequestPowerIrp(fdo, IRP_MN_SET_POWER, ps, setpowercallback, irp, NULL);

	if (res == STATUS_PENDING) {
		// then device state power irp has been sent successfully
		return STATUS_MORE_PROCESSING_REQUIRED;
	}


	IoReleaseRemoveLock(&pde->removelock, NULL);
	irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
	return STATUS_CONTINUE_COMPLETION;


}



extern "C" NTSTATUS powerhandler(PDEVICE_OBJECT fdo, PIRP irp) {

	

	PDEVICE_EXTENSION pde = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION iostacklocation = IoGetCurrentIrpStackLocation(irp);

	

	if (iostacklocation->MinorFunction == IRP_MN_QUERY_POWER) {


		KdPrint(("we got IRP_MN_QUERY_POWER\n"));

		POWER_STATE_TYPE powerstatetype = iostacklocation->Parameters.Power.Type;

		

		if (powerstatetype == SystemPowerState) {


			NTSTATUS res = IoAcquireRemoveLock(&pde->removelock, NULL);
			if (!NT_SUCCESS(res)) {
				PoStartNextPowerIrp(irp);
				IoSkipCurrentIrpStackLocation(irp);
				return PoCallDriver(pde->lowerdeviceobject, irp);
			}


			//SYSTEM_POWER_STATE systempowerstate =  iostacklocation->Parameters.Power.State.SystemState;

			PoStartNextPowerIrp(irp);

			IoCopyCurrentIrpStackLocationToNext(irp);
			IoSetCompletionRoutine(irp, querypowercompletionroutine, fdo, TRUE, TRUE, TRUE);
			return PoCallDriver(pde->lowerdeviceobject, irp);

		}


		else if (powerstatetype == DevicePowerState) {

			// this was sent by our fdo, device power policy owner
			NTSTATUS res = IoAcquireRemoveLock(&pde->removelock, NULL);
			if (!NT_SUCCESS(res)) {
				PoStartNextPowerIrp(irp);
				IoSkipCurrentIrpStackLocation(irp);
				return PoCallDriver(pde->lowerdeviceobject, irp);
			}


			DEVICE_POWER_STATE devicepowerstate =  iostacklocation->Parameters.Power.State.DeviceState;
			// our software device supports only D0 and D3
			if (devicepowerstate == PowerDeviceD3) {
				// can we shutdown ?

				if (InterlockedCompareExchange(&pde->iscriticaltaskrunning, 0, 0) == 1) {
					// critical task running, we cant shutdown
					PoStartNextPowerIrp(irp);
					IoReleaseRemoveLock(&pde->removelock, NULL);
					irp->IoStatus.Status = STATUS_DEVICE_BUSY;
					IoCompleteRequest(irp, IO_NO_INCREMENT);
					return STATUS_DEVICE_BUSY;
				}

			}
			PoStartNextPowerIrp(irp);
			IoReleaseRemoveLock(&pde->removelock, NULL);
			irp->IoStatus.Status = STATUS_SUCCESS;
			IoSkipCurrentIrpStackLocation(irp);
			return PoCallDriver(pde->lowerdeviceobject, irp);
		


		}


	}



	if (iostacklocation->MinorFunction == IRP_MN_SET_POWER) {

		POWER_STATE_TYPE powerstatetype = iostacklocation->Parameters.Power.Type;
		
		

		if (powerstatetype == SystemPowerState) {

			NTSTATUS res = IoAcquireRemoveLock(&pde->removelock, NULL);
			if (!NT_SUCCESS(res)) {
				PoStartNextPowerIrp(irp);
				IoSkipCurrentIrpStackLocation(irp);
				return PoCallDriver(pde->lowerdeviceobject, irp);
			}

			//SYSTEM_POWER_STATE systempowerstate = iostacklocation->Parameters.Power.State.SystemState;

			PoStartNextPowerIrp(irp);
			IoCopyCurrentIrpStackLocationToNext(irp);
			IoSetCompletionRoutine(irp, setpowercompletionroutine, fdo, TRUE, TRUE, TRUE);
			return PoCallDriver(pde->lowerdeviceobject, irp);


		}


		if (powerstatetype == DevicePowerState) {

			// fdo sent this to us
			NTSTATUS res = IoAcquireRemoveLock(&pde->removelock, NULL);
			if (!NT_SUCCESS(res)) {
				PoStartNextPowerIrp(irp);
				IoSkipCurrentIrpStackLocation(irp);
				return PoCallDriver(pde->lowerdeviceobject, irp);
			}
			DEVICE_POWER_STATE devicepowerstate = iostacklocation->Parameters.Power.State.DeviceState;

			if (devicepowerstate == PowerDeviceD3) {

				// we need to shutdown
				// stop accepting irps
				KIRQL oldirql;
				KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);
				pde->irpqueue.acceptirps = FALSE;
				KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);

				// cancelling remaining irps
				while (PIRP cancelirp = IoCsqRemoveNextIrp(&pde->csq, NULL)) {

					// cancelling the remaining irps
					IoReleaseRemoveLock(&pde->removelock, NULL);
					cancelirp->IoStatus.Status = STATUS_CANCELLED;
					cancelirp->IoStatus.Information = 0;
					IoCompleteRequest(cancelirp, IO_NO_INCREMENT);
					
				}

			
				POWER_STATE ps;
				ps.DeviceState = devicepowerstate;
				PoSetPowerState(fdo, DevicePowerState, ps);

			}

			else if (devicepowerstate == PowerDeviceD0) {
				// device is working
				// start accepting irps
				KIRQL oldirql;
				KeAcquireSpinLock(&pde->irpqueue.spinlock, &oldirql);
				pde->irpqueue.acceptirps = TRUE;
				KeReleaseSpinLock(&pde->irpqueue.spinlock, oldirql);

				POWER_STATE ps;
				ps.DeviceState = devicepowerstate;
				PoSetPowerState(fdo, DevicePowerState, ps);
			}


			IoReleaseRemoveLock(&pde->removelock, NULL);
			irp->IoStatus.Status = STATUS_SUCCESS;
			PoStartNextPowerIrp(irp);
			IoSkipCurrentIrpStackLocation(irp);
			return PoCallDriver(pde->lowerdeviceobject, irp);


		}

	}




	PoStartNextPowerIrp(irp);
	IoSkipCurrentIrpStackLocation(irp);
	return PoCallDriver(pde->lowerdeviceobject, irp);


}



