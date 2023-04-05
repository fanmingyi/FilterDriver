// ring3Demo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>


__declspec(naked) void GateSysIntCall() {
	__asm {
		lea edx, [ebp + 8];
		//调用29号中断
		int 29h
		ret
	}
}

void(*g_SysCall)() = &GateSysIntCall;

void SysCall1() {
	__asm {
		mov eax, 0;
		call g_SysCall
	}
}
void SysCall2(int p1) {

	__asm {
		mov eax, 1;
		call g_SysCall

	}
}


int main()
{
	SysCall1();


	printf("调用完毕 \r\n");
	


	getchar();
	return 0;
}

