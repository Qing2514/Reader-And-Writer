#include<iostream>
#include<string>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h> 
#include <fstream>
#include <io.h>
#include <string.h>
#include<algorithm>
#include<Windows.h> //多线程编程
#include<process.h>
#include <tchar.h>
using namespace std;

#define READER 'R'                   //读者
#define WRITER 'W'                   //写者
#define INTE_PER_SEC 1000            //每秒时钟中断的数目
#define MAX_THREAD_NUM 64         //最大线程数目

//变量声明初始化
int readercount = 0;//记录等待的读者数目
int writercount = 0;//记录等待的写者数目

HANDLE rcsignal;//因为读者数量而添加的互斥信号量，用于读者优先

HANDLE RCSignal;//因为读者数量而添加的互斥信号量，用于写者优先
HANDLE writeCountSignal;//因为写者数量而添加的互斥信号量
HANDLE filesrc;//互斥访问信号量
HANDLE wrt;
//保证每次只有一个写者进行写操作，当写者的数量writercount等于0的时候，则证明此时没有没有读者了,释放信号量filesrc
HANDLE read_s;//进程优先互斥
struct thread_info {
	int id;		      //线程序号
	char type;      //线程类别(判断是读者线程还是写者线程)
	double delay;		 //线程延迟时间
	double lastTime;	 //线程读写操作时间
};

//读者优先
//进程管理-读者线程
void rp_threadReader(void* p)
{
	DWORD delaytime;                   //延迟时间
	DWORD duration;                 //读文件持续时间
	int num_thread;                    //线程序号
									 //从参数中获得信息
	num_thread = ((thread_info*)(p))->id;
	delaytime = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);
	duration = (DWORD)(((thread_info*)(p))->lastTime * INTE_PER_SEC);
	Sleep(delaytime);                  //延迟等待
	cout << "读者线程" << num_thread << "申请读文件." << endl;
	WaitForSingleObject(rcsignal, -1);
	//申请互斥信号量rcsignal，多个读者对readercount互斥访问
	if (readercount == 0)WaitForSingleObject(filesrc, -1);
	//第一位读者申请书,仅当readercount为0时，表示此时无读者，reader进程才需要执行WaitForSingleObject
	readercount++;
	ReleaseSemaphore(rcsignal, 1, NULL);//释放互斥信号量rcsignal
	cout << "读者线程" << num_thread << "开始读文件." << endl;
	Sleep(duration);
	cout << "读者线程" << num_thread << "完成读文件." << endl;
	WaitForSingleObject(rcsignal, -1);//修改readercount
	readercount--;//读者读完
	if (readercount == 0)ReleaseSemaphore(filesrc, 1, NULL);//释放书籍，写者可写
	ReleaseSemaphore(rcsignal, 1, NULL);//释放互斥信号量rcsignal
}
//读者优先
//进程管理-写者线程
void rp_threadWriter(void* p)
{
	DWORD delaytime;                   //延迟时间
	DWORD duration;                 //读文件持续时间
	int num_thread;                    //线程序号
									 //从参数中获得信息
	num_thread = ((thread_info*)(p))->id;
	delaytime = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);
	duration = (DWORD)(((thread_info*)(p))->lastTime * INTE_PER_SEC);
	Sleep(delaytime);                  //延迟等待
	cout << "写者线程" << num_thread << "申请写文件." << endl;
	WaitForSingleObject(filesrc, INFINITE);//申请资源
	cout << "写者线程" << num_thread << "开始写文件." << endl;
	Sleep(duration);
	cout << "写者线程" << num_thread << "完成写文件." << endl;
	ReleaseSemaphore(filesrc, 1, NULL);//释放资源
}
//读者优先
void ReaderPriority()
{
	DWORD n_thread = 0;           //线程数目
	DWORD thread_ID;            //线程ID
	DWORD wait_for_all;         //等待所有线程结束
	rcsignal = CreateSemaphore(NULL, 1, 1, _T("mutex_for_readcount"));// 创建信号量对象 读者对count修改互斥信号量，初值为1,最大为1   
	//（lpSemaphoreAttributes [in，optional]指向SECURITY_ATTRIBUTES结构的指针。如果此参数为NULL，则子进程无法继承句柄。、初值、最大值、信号量对象名称）
	filesrc = CreateSemaphore(NULL, 1, 1, NULL);//书籍互斥访问信号量，初值为1,最大值为1
	HANDLE h_Thread[MAX_THREAD_NUM];//线程句柄,线程对象的数组
	thread_info thread_info[MAX_THREAD_NUM];//保存线程信息的数组
	int id = 0;
	readercount = 0;               //初始化readcount
	cout << "读者优先:" << endl;
	fstream file;
	file.open("数据.txt", ios::in);
	int id1;
	char type1;
	double delay1, lastTime1;
	while (!file.eof()) {
		file >> id1;
		file >> type1;
		file >> delay1;
		file >> lastTime1;
		thread_info[n_thread].id = id1;
		thread_info[n_thread].type = type1;
		thread_info[n_thread].delay = delay1;
		thread_info[n_thread++].lastTime = lastTime1;
	}
	for (int i = 0; i < (int)(n_thread); i++)
	{
		if (thread_info[i].type == READER || thread_info[i].type == 'R') {//线程类别是读者  //安全设置、堆栈大小、入口函数指针、函数参数、启动选项、输出线程 ID ，返回线程句柄
			//创建读者进程
			h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(rp_threadReader), &thread_info[i], 0, &thread_ID);
		}
		else {
			//创建写线程
			h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(rp_threadWriter), &thread_info[i], 0, &thread_ID);
		}
	}
	//等待子线程结束
	//关闭句柄
	wait_for_all = WaitForMultipleObjects(n_thread, h_Thread, TRUE, -1);  //等待所有线程结束
	cout << endl;
	cout << "所有读者写者已经完成操作！" << endl;
	for (int i = 0; i < (int)(n_thread); i++)
		CloseHandle(h_Thread[i]);
	CloseHandle(rcsignal);
	CloseHandle(filesrc);
}
//写者优先
//进程管理-读者线程
void wp_threadReader(void* p) {
	DWORD delaytime;                   //延迟时间
	DWORD duration;                 //读文件持续时间
	int num_thread;                    //线程序号
									 //从参数中获得信息
	num_thread = ((thread_info*)(p))->id;
	delaytime = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);
	duration = (DWORD)(((thread_info*)(p))->lastTime * INTE_PER_SEC);
	Sleep(delaytime);                  //延迟等待
	//printf("读者进程%d申请读文件.\n", num_thread);
	cout << "读者线程" << num_thread << "申请读文件." << endl;
	WaitForSingleObject(read_s, -1);   //申请读取权限
	WaitForSingleObject(RCSignal, -1);//对readercount互斥访问，即多个读者互斥
	if (readercount == 0)
		WaitForSingleObject(wrt, -1);
	//第一位读者申请书,同时防止写者进行写操作
	readercount++;
	ReleaseSemaphore(RCSignal, 1, NULL);//释放互斥信号量rcsignal
	ReleaseSemaphore(read_s, 1, NULL);//释放互斥信号量read
									 /* reading is performed */
	cout << "读者线程" << num_thread << "开始读文件." << endl;
	Sleep(duration);
	cout << "读者线程" << num_thread << "完成读文件." << endl;
	WaitForSingleObject(RCSignal, -1);//修改readercount
	readercount--;//读者读完
	if (readercount == 0)
		ReleaseSemaphore(wrt, 1, NULL);//释放资源，写者可写
	ReleaseSemaphore(RCSignal, 1, NULL);//释放互斥信号量rcsignal
}
//写者优先
//进程管理-写者线程
void wp_threadWriter(void* p) {
	DWORD delaytime;                   //延迟时间
	DWORD duration;                 //读文件持续时间
	int num_thread;                    //线程序号
									 //从参数中获得信息
	num_thread = ((thread_info*)(p))->id;
	delaytime = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);
	duration = (DWORD)(((thread_info*)(p))->lastTime * INTE_PER_SEC);
	Sleep(delaytime);                  //延迟等待
	cout << "写者线程" << num_thread << "申请写文件." << endl;
	WaitForSingleObject(writeCountSignal, -1);//对writercount互斥访问,申请写者计数器资源
	if (writercount == 0)
		WaitForSingleObject(read_s, -1);
	//第一位写者申请资源
	writercount++;
	ReleaseSemaphore(writeCountSignal, 1, NULL);//释放写者计数器资源
	WaitForSingleObject(wrt, -1);//申请文件资源
	/*write is performed*/
	cout << "写者线程" << num_thread << "开始写文件." << endl;
	Sleep(duration);
	cout << "写者线程" << num_thread << "完成写文件." << endl;
	ReleaseSemaphore(wrt, 1, NULL);//释放资源
	WaitForSingleObject(writeCountSignal, -1);//对writercount互斥访问，即多个写者互斥
	writercount--;
	if (writercount == 0)
		ReleaseSemaphore(read_s, 1, NULL);
	//释放资源
	ReleaseSemaphore(writeCountSignal, 1, NULL);//释放资源
}
//写者优先
void WriterPriority() {
	DWORD n_thread = 0;           //线程数目
	DWORD thread_ID;            //线程ID
	DWORD wait_for_all;         //等待所有线程结束
								//创建信号量
	RCSignal = CreateSemaphore(NULL, 1, 1, _T("mutex_for_readercount"));//读者对count修改互斥信号量，初值为1,最大为1
	writeCountSignal = CreateSemaphore(NULL, 1, 1, _T("mutex_for_writercount"));//写者对count修改互斥信号量，初值为1,最大为1
	wrt = CreateSemaphore(NULL, 1, 1, NULL);//
	read_s = CreateSemaphore(NULL, 1, 1, NULL);//书籍互斥访问信号量，初值为1,最大值为1
	HANDLE h_Thread[MAX_THREAD_NUM];//线程句柄,线程对象的数组
	thread_info thread_info[MAX_THREAD_NUM];//保存线程信息的数组
	int id = 0;
	readercount = 0;               //初始化readcount
	writercount = 0;
	cout << "写者优先:" << endl;
	fstream file;
	file.open("数据.txt", ios::in);
	int id1;
	char type1;
	double delay1, lastTime1;
	while (!file.eof()) {
		file >> id1;
		file >> type1;
		file >> delay1;
		file >> lastTime1;
		thread_info[n_thread].id = id1;
		thread_info[n_thread].type = type1;
		thread_info[n_thread].delay = delay1;
		thread_info[n_thread++].lastTime = lastTime1;
	}
	for (int i = 0; i < (int)(n_thread); i++)
	{
		if (thread_info[i].type == READER || thread_info[i].type == 'r')
		{
			//创建读者进程
			h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(wp_threadReader), &thread_info[i], 0, &thread_ID);
		}
		else
		{
			//创建写线程
			h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(wp_threadWriter), &thread_info[i], 0, &thread_ID);
		}
	}
	//等待子线程结束
	//关闭句柄
	wait_for_all = WaitForMultipleObjects(n_thread, h_Thread, TRUE, -1);
	cout << endl;
	cout << "所有读者写者已经完成操作！！" << endl;
	for (int i = 0; i < (int)(n_thread); i++)
		CloseHandle(h_Thread[i]);
	CloseHandle(writeCountSignal);
	CloseHandle(RCSignal);
	CloseHandle(wrt);
}
//主函数
int main()
{
	char choice;
	//cout << "    读写操作测试程序    " << endl;
	cout << endl;
	while (true)
	{
		//打印提示信息
		cout << "请按照需求输入序号：       " << endl;
		cout << "1、读者优先" << endl;
		cout << "2、写者优先" << endl;
		cout << "3、退出程序" << endl;
		cout << endl;
		//如果输入信息不正确，继续输入
		do {
			choice = (char)_getch();
		} while (choice != '1' && choice != '2' && choice != '3');
		system("cls");//清屏
		//选择1，读者优先
		if (choice == '1') {
			//ReaderPriority("thread.txt");
			ReaderPriority();
			system("pause");
		}
		//选择2，写者优先
		else if (choice == '2') {
			WriterPriority();
			system("pause");
		}
		//选择3，退出
		else
			return 0;
		//结束
		cout << "\nPress Any Key to Coutinue";
		_getch();
		system("cls");
	}
	return 0;
}