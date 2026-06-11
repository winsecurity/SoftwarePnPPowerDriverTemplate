#pragma once
#include <ntddk.h>
#include "definitions.h"
#include "irpdrainer.h"


void csqacquirelock(PIO_CSQ csq, PKIRQL irql);


void csqreleaselock(PIO_CSQ csq, KIRQL irql);

void csqinsertirp(PIO_CSQ csq, PIRP irp);
NTSTATUS csqinsertirpex(PIO_CSQ csq, PIRP irp, PVOID insertcontext);



void csqremoveirp(PIO_CSQ csq, PIRP irp);


void csqcompletecanceledirp(PIO_CSQ csq, PIRP irp);


PIRP csqpeeknextirp(PIO_CSQ csq, PIRP irp, PVOID peekcontext);
