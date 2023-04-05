
#include <Ntddk.h>
#include<ntddkbd.h>


LONG g_nRefCount = 0;
PDRIVER_OBJECT  g_pDriverObject;
PDEVICE_OBJECT  g_pNextDevice = NULL;

PDRIVER_DISPATCH g_OldMajorFunction[IRP_MJ_MAXIMUM_FUNCTION];


NTSTATUS DispatchCommand(
	_In_ struct _DEVICE_OBJECT*  DeviceObject,
	_Inout_ struct _IRP* Irp
) {

	DbgPrint("[Myattach] enter DispatchCommand \r\n");

	UNREFERENCED_PARAMETER(DeviceObject);
	IoCopyCurrentIrpStackLocationToNext(Irp);
	DbgPrint("[Myattach] exit DispatchCommand \r\n");

	return IoCallDriver(g_pNextDevice, Irp);
}

NTSTATUS
IoCompletionRoutine(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp,
	_In_reads_opt_(_Inexpressible_("varies")) PVOID Context
) {

	DbgPrint("[Myattach] enter IoCompletionRoutine \r\n");

	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Context);


	if (Irp->PendingReturned)
	{
		DbgPrint("[Myattach] IoCompletionRoutine  PendingReturned \r\n");

		IoMarkIrpPending(Irp);
	}
	InterlockedDecrement(&g_nRefCount);
	DbgPrint("[Myattach] exit IoCompletionRoutine \r\n");

	return Irp->IoStatus.Status;

}
NTSTATUS DispatchRead(
	_In_ struct _DEVICE_OBJECT*  DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	DbgPrint("[Myattach]  DispatchRead \r\n");


	//NTSTATUS status = (g_OldMajorFunction[IRP_MJ_READ])(DeviceObject, Irp);
	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, &IoCompletionRoutine, NULL, true, true, true);
	InterlockedIncrement(&g_nRefCount);
	return IoCallDriver(g_pNextDevice, Irp);


}
void detachOther() {
	IoDetachDevice(g_pNextDevice);
}

//这个函数被注册用于驱动卸载调用
VOID myUnload(struct _DRIVER_OBJECT* DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);
	DbgPrint("[Myattach]  enter myUnload \r\n");



	//解除附加驱动
	detachOther();

	DbgPrint("[Myattach]  myUnload g_nRefCount %d \r\n", g_nRefCount);



	while (g_nRefCount != 0);
	DbgPrint("[Myattach]  myUnload done %d \r\n");

	if (DriverObject->DeviceObject != NULL)
	{
		IoDeleteDevice(DriverObject->DeviceObject);
	}
	DbgPrint("[Myattach]  exit myUnload \r\n");


}
void attachOtherDriver(PDRIVER_OBJECT pDriverObject) {
	DbgPrint("[Myattach]  enter attachOtherDriver \r\n ");


	UNICODE_STRING  ustrDevName;
	PFILE_OBJECT pFileObject = NULL;
	PDEVICE_OBJECT pTargetDevice = NULL;
	RtlInitUnicodeString(&ustrDevName, L"\\Device\\MyDDKName");
	//获取命名设备的设备对象 并传入到DeviceObject
	NTSTATUS  Status = IoGetDeviceObjectPointer(&ustrDevName, FILE_ALL_ACCESS, &pFileObject, &pTargetDevice);

	DbgPrint("[Myattach]  pTargetDevice:%p \r\n", pTargetDevice);

	if (!NT_SUCCESS(Status))
	{
		//获取设备对象对应的驱动
		DbgPrint("[Myattach] HookKeyboard failure IoGetDeviceObjectPointer \r\n");
		return;
	}



	//创建设备
	UNICODE_STRING  ustrMyDevName;
	RtlInitUnicodeString(&ustrMyDevName, L"\\Device\\Myattach");
	PDEVICE_OBJECT pSourceDevice = NULL;
	Status = IoCreateDevice(pDriverObject, 0, &ustrMyDevName, pTargetDevice->DeviceType, pTargetDevice->Characteristics, FALSE, &pSourceDevice);


	if (!NT_SUCCESS(Status)) {
		DbgPrint("[Myattach] HookKeyboard failure IoCreateDevice \r\n");
		return;
	}

	pSourceDevice->Flags = pTargetDevice->Flags;
	//执行附加
	g_pNextDevice = IoAttachDeviceToDeviceStack(pSourceDevice, pTargetDevice);

	DbgPrint("[My learning] HookKeyboard  g_pNextDevice  %p \r\n", g_pNextDevice);

	if (g_pNextDevice == NULL)
	{
		IoDeleteDevice(pSourceDevice);
	}

	DbgPrint("[Myattach]  exit attachOtherDriver \r\n ");

}


//驱动被加载的时候会调用此函数
extern "C"
NTSTATUS
DriverEntry(
	_In_ struct _DRIVER_OBJECT* DriverObject,
	_In_ PUNICODE_STRING    RegistryPath
)
{

	DbgPrint("[Myattach]  enter DriverEntry \r\n ");

	//如果你没有用到参数需要告诉系统。
	UNREFERENCED_PARAMETER(RegistryPath);

	//设置相关回调
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
	{
		DriverObject->MajorFunction[i] = &DispatchCommand;
	}
	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	//附加到其他驱动上
	attachOtherDriver(DriverObject);

	//设置卸载
	DriverObject->DriverUnload = myUnload;
	
	DbgPrint("[Myattach]  exit DriverEntry \r\n ");

	return STATUS_SUCCESS;
}
