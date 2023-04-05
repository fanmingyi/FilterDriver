#include <ntddk.h>

LIST_ENTRY ListHead;
VOID Unload(PDRIVER_OBJECT driver)
{

	DbgPrint("[MyDDKName] enter Unload \r\n");

	UNICODE_STRING SymbolicLinkName = RTL_CONSTANT_STRING(L"\\??\\MyDDKLink");
	IoDeleteSymbolicLink(&SymbolicLinkName);
	IoDeleteDevice(driver->DeviceObject);

	DbgPrint("[MyDDKName] exit Unload \r\n");

}

NTSTATUS Create(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	DbgPrint("[MyDDKName] enter Create \r\n");

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	DbgPrint("[MyDDKName] exit Create \r\n");

	return STATUS_SUCCESS;
}


NTSTATUS Read(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	DbgPrint("[MyDDKName] enter read \r\n");

	IoMarkIrpPending(Irp);

	InsertHeadList(&ListHead, &Irp->Tail.Overlay.ListEntry);//将Irp插入链表//
	
	DbgPrint("[MyDDKName] exit read \r\n");

	return STATUS_PENDING;;
}

NTSTATUS CleanUp(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	DbgPrint("[MyDDKName] enter CleanUp \r\n");
	while (!IsListEmpty(&ListHead))
	{
		PLIST_ENTRY pEntry = RemoveHeadList(&ListHead);
		PIRP pendingIrp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);//根据结构体某一成员的地址获取结构体的基地址//

		pendingIrp->IoStatus.Status = STATUS_SUCCESS;
		pendingIrp->IoStatus.Information = 0;
		IoCompleteRequest(pendingIrp, IO_NO_INCREMENT);

		DbgPrint("[MyDDKName] done an IRP  pending  \r\n");

	}


	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	DbgPrint("[MyDDKName] exit CleanUp \r\n");

	return STATUS_SUCCESS;
}


NTSTATUS DispatchControl(
	_In_ struct _DEVICE_OBJECT*  DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	DbgPrint("[MyDDKName] enter DispatchControl \r\n");
	CleanUp(DeviceObject, Irp);
	DbgPrint("[MyDDKName] exit DispatchControl \r\n");

	return STATUS_SUCCESS;
}
NTSTATUS Close(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	DbgPrint("[MyDDKName] enter Close\n");
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	DbgPrint("[MyDDKName]  exit Close\n");
	return STATUS_SUCCESS;
}
NTSTATUS DriverEntry(PDRIVER_OBJECT driver)
{
	UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\MyDDKName");
	UNICODE_STRING SymbolicLinkName = RTL_CONSTANT_STRING(L"\\??\\MyDDKLink");

	PDEVICE_OBJECT DeviceObject;
	DbgPrint("[MyDDKName]  enter DriverEntry\n");

	NTSTATUS status;
	status = IoCreateDevice(driver, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);

	if (!NT_SUCCESS(status))
	{
		DbgPrint("[MyDDKName]  IoCreateDevice error\n");

		return status;
	}
	status = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("[MyDDKName]  IoCreateSymbolicLink error\n");
		IoDeleteDevice(DeviceObject);
		return status;
	}
	InitializeListHead(&ListHead);
	driver->MajorFunction[IRP_MJ_CREATE] = Create;
	driver->MajorFunction[IRP_MJ_READ] = Read;
	driver->MajorFunction[IRP_MJ_CLEANUP] = CleanUp;
	driver->MajorFunction[IRP_MJ_CLOSE] = Close;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchControl;

	DeviceObject->Flags |= DO_BUFFERED_IO;
	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;




	driver->DriverUnload = Unload;
	DbgPrint("[MyDDKName]  exit DriverEntry\n");

	return STATUS_SUCCESS;
}
