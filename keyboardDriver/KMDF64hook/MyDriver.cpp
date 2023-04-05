
#include <Ntddk.h>
#include<ntddkbd.h>

//ϵͳ����API ֻ��Ҫ��������
extern "C" UCHAR * PsGetProcessImageFileName(PEPROCESS Process);


LONG g_nRefCount = 0;
PDRIVER_OBJECT  g_pDriverObject;
PDEVICE_OBJECT  g_pNextDevice = NULL;

PDRIVER_DISPATCH g_OldMajorFunction[IRP_MJ_MAXIMUM_FUNCTION];


NTSTATUS DispatchCommand(
	_In_ struct _DEVICE_OBJECT*  DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	//DbgPrint("[My learning] DispatchCommand \r\n");
	//InterlockedIncrement(&g_nRefCount);
	////NTSTATUS status = (g_OldMajorFunction[IRP_MJ_READ])(DeviceObject, Irp);
	//InterlockedDecrement(&g_nRefCount);

	IoCopyCurrentIrpStackLocationToNext(Irp);
	return IoCallDriver(g_pNextDevice, Irp);
}

NTSTATUS
IoCompletionRoutine(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_ PIRP Irp,
	_In_reads_opt_(_Inexpressible_("varies")) PVOID Context
) {
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Context);
	DbgPrint("[My learning] IoCompletionRoutine \r\n");


	//PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(Irp);
	PVOID pBuffer = Irp->AssociatedIrp.SystemBuffer;
	auto nLength = Irp->IoStatus.Information;
	for (ULONG_PTR i = 0; i < nLength/sizeof(KEYBOARD_INPUT_DATA); i++)
	{
		KEYBOARD_INPUT_DATA *pInputData = (KEYBOARD_INPUT_DATA *)pBuffer;
		DbgPrint("[My learning] DispatchRead IRQL %d pBuffer:%p  nLength: %d   UnitId %d MakeCode %d Flags %d\r\n",
			KeGetCurrentIrql(),
			pBuffer,
			nLength,
			pInputData[i].UnitId,
			pInputData[i].MakeCode,
			pInputData[i].Flags
			);
	}
	
	if (Irp->PendingReturned)
	{
		DbgPrint("[My learning] IoCompletionRoutine  PendingReturned \r\n");

		IoMarkIrpPending(Irp);
	}
	InterlockedDecrement(&g_nRefCount);
	return Irp->IoStatus.Status;

}




NTSTATUS DispatchRead(
	_In_ struct _DEVICE_OBJECT*  DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	UNREFERENCED_PARAMETER(DeviceObject);

	DbgPrint("[My learning] DispatchRead \r\n");




	//NTSTATUS status = (g_OldMajorFunction[IRP_MJ_READ])(DeviceObject, Irp);
	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, &IoCompletionRoutine, NULL, true, true, true);
	InterlockedIncrement(&g_nRefCount);
	return IoCallDriver(g_pNextDevice, Irp);
}
void UnHookKeyboard() {
	IoDetachDevice(g_pNextDevice);
}

//���������ע����������ж�ص���
VOID myUnload(struct _DRIVER_OBJECT* DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);
	DbgPrint("[My learning]  drive myUnload \r\n");




	UnHookKeyboard();




	DbgPrint("[My learning]  myUnload g_nRefCount %d \r\n", g_nRefCount);



	while (g_nRefCount != 0);
	DbgPrint("[My learning]  myUnload done %d \r\n");

	if (DriverObject->DeviceObject != NULL)
	{
		IoDeleteDevice(DriverObject->DeviceObject);
	}


}
void HookKeyboard(PDRIVER_OBJECT pDriverObject) {


	//��ȡԭʼ�ļ����豸
	UNICODE_STRING  ustrDevName;
	PFILE_OBJECT pFileObject = NULL;
	PDEVICE_OBJECT pTargetDevice = NULL;
	RtlInitUnicodeString(&ustrDevName, L"\\Device\\KeyboardClass0");
	//��ȡ�����豸���豸���� �����뵽DeviceObject
	NTSTATUS  Status = IoGetDeviceObjectPointer(&ustrDevName, FILE_ALL_ACCESS, &pFileObject, &pTargetDevice);

	DbgPrint("[My learning]  pTargetDevice:%p \r\n", pTargetDevice);

	if (!NT_SUCCESS(Status))
	{
		//��ȡ�豸�����Ӧ������
		DbgPrint("[My learning] HookKeyboard failure IoGetDeviceObjectPointer \r\n");
		return;
	}




	UNICODE_STRING  ustrMyDevName;
	RtlInitUnicodeString(&ustrMyDevName, L"\\Device\\MyKeyboard");
	PDEVICE_OBJECT pSourceDevice = NULL;
	Status = IoCreateDevice(pDriverObject, 0, &ustrMyDevName, pTargetDevice->DeviceType, pTargetDevice->Characteristics, FALSE, &pSourceDevice);


	if (!NT_SUCCESS(Status)) {
		DbgPrint("[My learning] HookKeyboard failure IoCreateDevice \r\n");
		return;
	}

	pSourceDevice->Flags = pTargetDevice->Flags;

	g_pNextDevice = IoAttachDeviceToDeviceStack(pSourceDevice, pTargetDevice);

	DbgPrint("[My learning] HookKeyboard  g_pNextDevice  %p \r\n", g_pNextDevice);

	if (g_pNextDevice == NULL)
	{

		IoDeleteDevice(pSourceDevice);
	}


}


//���������ص�ʱ�����ô˺���
extern "C"
NTSTATUS
DriverEntry(
	_In_ struct _DRIVER_OBJECT* DriverObject,
	_In_ PUNICODE_STRING    RegistryPath
)
{

	//��ӡ��Ϣ
	DbgPrint("[My learning]  drive loaded\r\n ");

	//�����û���õ�������Ҫ����ϵͳ��
	UNREFERENCED_PARAMETER(RegistryPath);
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION + 1; i++)
	{
		//�滻�ص�
		DriverObject->MajorFunction[i] = &DispatchCommand;
	}
	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	HookKeyboard(DriverObject);

	DriverObject->DriverUnload = myUnload;

	return STATUS_SUCCESS;
}
