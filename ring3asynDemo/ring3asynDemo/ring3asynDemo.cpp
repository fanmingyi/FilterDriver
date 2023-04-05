// syn_asy三环代码.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <stdio.h>
#include <Windows.h>

using namespace std;
using std::count;
using std::endl;

int main()
{
	HANDLE hDevice = CreateFile(TEXT("\\\\.\\MyDDKLink"),
		GENERIC_ALL,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("打开设备失败 %d\n", GetLastError());
		return -1;
	}
	DWORD dwret;
	OVERLAPPED overlapped = { NULL };

	int flag = -1;

	while (true)
	{
		cout << "输入1: 读取驱动数据 \r\n 输入2 通知内核完成驱动异步过程" << endl;
		cin >> flag;
		if (flag == 1)
		{
			BOOL result = ReadFile(hDevice, NULL, 0, &dwret, &overlapped);

			cout << " 读取驱动数据完成 dwret " << dwret << " result " << result << endl;

			if (!result && GetLastError() == ERROR_IO_PENDING) {

				cout << "!result && GetLastError() == ERROR_IO_PENDING true " << endl;
				// I/O is pending, wait for completion
				/*result = GetOverlappedResult(
					hDevice,
					&overlapped,
					NULL,
					INFINITE
				);*/
			}
			/*if (result) {
				cout << "Write file success." << endl;
			}
			else {
				cerr << "Write file failed." << endl;
			}*/
		}
		else if (flag == 2)
		{

			BOOL result = DeviceIoControl(hDevice, 1, nullptr, 0, nullptr, 0, nullptr, &overlapped);
			if (!result && GetLastError() == ERROR_IO_PENDING) {

				cout << "DeviceIoControl !result && GetLastError() == ERROR_IO_PENDING true " << endl;
				//I/O is pending, wait for completion
				result = GetOverlappedResult(
					hDevice,
					&overlapped,
					NULL,
					INFINITE
				);
			}
			else if (result)
			{
				cout << "DeviceIoControl success " << endl;

			}
		}
		else
		{
			break;
		}


	}


	cout << "执行完成 CloseHandle" << endl;

	CloseHandle(hDevice);//在内核会触发 MJ_cleanuP 与Close派遣函数//

	getchar();
	return 0;
}
