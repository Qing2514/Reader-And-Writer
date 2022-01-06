#include<iostream>
#include<string>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h> 
#include <fstream>
#include <io.h>
#include <string.h>
#include<algorithm>
#include<Windows.h> //���̱߳��
#include<process.h>
#include <tchar.h>
using namespace std;

#define READER 'R'                   //����
#define WRITER 'W'                   //д��
#define INTE_PER_SEC 1000            //ÿ��ʱ���жϵ���Ŀ
#define MAX_THREAD_NUM 64         //����߳���Ŀ

//����������ʼ��
int readercount = 0;//��¼�ȴ��Ķ�����Ŀ
int writercount = 0;//��¼�ȴ���д����Ŀ

HANDLE rcsignal;//��Ϊ������������ӵĻ����ź��������ڶ�������

HANDLE RCSignal;//��Ϊ������������ӵĻ����ź���������д������
HANDLE writeCountSignal;//��Ϊд����������ӵĻ����ź���
HANDLE filesrc;//��������ź���
HANDLE wrt;
//��֤ÿ��ֻ��һ��д�߽���д��������д�ߵ�����writercount����0��ʱ����֤����ʱû��û�ж�����,�ͷ��ź���filesrc
HANDLE read_s;//�������Ȼ���
struct thread_info {
	int id;		      //�߳����
	char type;      //�߳����(�ж��Ƕ����̻߳���д���߳�)
	double delay;		 //�߳��ӳ�ʱ��
	double lastTime;	 //�̶߳�д����ʱ��
};

//��������
//���̹���-�����߳�
void rp_threadReader(void* p)
{
	DWORD delaytime;                   //�ӳ�ʱ��
	DWORD duration;                 //���ļ�����ʱ��
	int num_thread;                    //�߳����
									 //�Ӳ����л����Ϣ
	num_thread = ((thread_info*)(p))->id;
	delaytime = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);
	duration = (DWORD)(((thread_info*)(p))->lastTime * INTE_PER_SEC);
	Sleep(delaytime);                  //�ӳٵȴ�
	cout << "�����߳�" << num_thread << "������ļ�." << endl;
	WaitForSingleObject(rcsignal, -1);
	//���뻥���ź���rcsignal��������߶�readercount�������
	if (readercount == 0)WaitForSingleObject(filesrc, -1);
	//��һλ����������,����readercountΪ0ʱ����ʾ��ʱ�޶��ߣ�reader���̲���Ҫִ��WaitForSingleObject
	readercount++;
	ReleaseSemaphore(rcsignal, 1, NULL);//�ͷŻ����ź���rcsignal
	cout << "�����߳�" << num_thread << "��ʼ���ļ�." << endl;
	Sleep(duration);
	cout << "�����߳�" << num_thread << "��ɶ��ļ�." << endl;
	WaitForSingleObject(rcsignal, -1);//�޸�readercount
	readercount--;//���߶���
	if (readercount == 0)ReleaseSemaphore(filesrc, 1, NULL);//�ͷ��鼮��д�߿�д
	ReleaseSemaphore(rcsignal, 1, NULL);//�ͷŻ����ź���rcsignal
}
//��������
//���̹���-д���߳�
void rp_threadWriter(void* p)
{
	DWORD delaytime;                   //�ӳ�ʱ��
	DWORD duration;                 //���ļ�����ʱ��
	int num_thread;                    //�߳����
									 //�Ӳ����л����Ϣ
	num_thread = ((thread_info*)(p))->id;
	delaytime = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);
	duration = (DWORD)(((thread_info*)(p))->lastTime * INTE_PER_SEC);
	Sleep(delaytime);                  //�ӳٵȴ�
	cout << "д���߳�" << num_thread << "����д�ļ�." << endl;
	WaitForSingleObject(filesrc, INFINITE);//������Դ
	cout << "д���߳�" << num_thread << "��ʼд�ļ�." << endl;
	Sleep(duration);
	cout << "д���߳�" << num_thread << "���д�ļ�." << endl;
	ReleaseSemaphore(filesrc, 1, NULL);//�ͷ���Դ
}
//��������
void ReaderPriority()
{
	DWORD n_thread = 0;           //�߳���Ŀ
	DWORD thread_ID;            //�߳�ID
	DWORD wait_for_all;         //�ȴ������߳̽���
	rcsignal = CreateSemaphore(NULL, 1, 1, _T("mutex_for_readcount"));// �����ź������� ���߶�count�޸Ļ����ź�������ֵΪ1,���Ϊ1   
	//��lpSemaphoreAttributes [in��optional]ָ��SECURITY_ATTRIBUTES�ṹ��ָ�롣����˲���ΪNULL�����ӽ����޷��̳о��������ֵ�����ֵ���ź����������ƣ�
	filesrc = CreateSemaphore(NULL, 1, 1, NULL);//�鼮��������ź�������ֵΪ1,���ֵΪ1
	HANDLE h_Thread[MAX_THREAD_NUM];//�߳̾��,�̶߳��������
	thread_info thread_info[MAX_THREAD_NUM];//�����߳���Ϣ������
	int id = 0;
	readercount = 0;               //��ʼ��readcount
	cout << "��������:" << endl;
	fstream file;
	file.open("����.txt", ios::in);
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
		if (thread_info[i].type == READER || thread_info[i].type == 'R') {//�߳�����Ƕ���  //��ȫ���á���ջ��С����ں���ָ�롢��������������ѡ�����߳� ID �������߳̾��
			//�������߽���
			h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(rp_threadReader), &thread_info[i], 0, &thread_ID);
		}
		else {
			//����д�߳�
			h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(rp_threadWriter), &thread_info[i], 0, &thread_ID);
		}
	}
	//�ȴ����߳̽���
	//�رվ��
	wait_for_all = WaitForMultipleObjects(n_thread, h_Thread, TRUE, -1);  //�ȴ������߳̽���
	cout << endl;
	cout << "���ж���д���Ѿ���ɲ�����" << endl;
	for (int i = 0; i < (int)(n_thread); i++)
		CloseHandle(h_Thread[i]);
	CloseHandle(rcsignal);
	CloseHandle(filesrc);
}
//д������
//���̹���-�����߳�
void wp_threadReader(void* p) {
	DWORD delaytime;                   //�ӳ�ʱ��
	DWORD duration;                 //���ļ�����ʱ��
	int num_thread;                    //�߳����
									 //�Ӳ����л����Ϣ
	num_thread = ((thread_info*)(p))->id;
	delaytime = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);
	duration = (DWORD)(((thread_info*)(p))->lastTime * INTE_PER_SEC);
	Sleep(delaytime);                  //�ӳٵȴ�
	//printf("���߽���%d������ļ�.\n", num_thread);
	cout << "�����߳�" << num_thread << "������ļ�." << endl;
	WaitForSingleObject(read_s, -1);   //�����ȡȨ��
	WaitForSingleObject(RCSignal, -1);//��readercount������ʣ���������߻���
	if (readercount == 0)
		WaitForSingleObject(wrt, -1);
	//��һλ����������,ͬʱ��ֹд�߽���д����
	readercount++;
	ReleaseSemaphore(RCSignal, 1, NULL);//�ͷŻ����ź���rcsignal
	ReleaseSemaphore(read_s, 1, NULL);//�ͷŻ����ź���read
									 /* reading is performed */
	cout << "�����߳�" << num_thread << "��ʼ���ļ�." << endl;
	Sleep(duration);
	cout << "�����߳�" << num_thread << "��ɶ��ļ�." << endl;
	WaitForSingleObject(RCSignal, -1);//�޸�readercount
	readercount--;//���߶���
	if (readercount == 0)
		ReleaseSemaphore(wrt, 1, NULL);//�ͷ���Դ��д�߿�д
	ReleaseSemaphore(RCSignal, 1, NULL);//�ͷŻ����ź���rcsignal
}
//д������
//���̹���-д���߳�
void wp_threadWriter(void* p) {
	DWORD delaytime;                   //�ӳ�ʱ��
	DWORD duration;                 //���ļ�����ʱ��
	int num_thread;                    //�߳����
									 //�Ӳ����л����Ϣ
	num_thread = ((thread_info*)(p))->id;
	delaytime = (DWORD)(((thread_info*)(p))->delay * INTE_PER_SEC);
	duration = (DWORD)(((thread_info*)(p))->lastTime * INTE_PER_SEC);
	Sleep(delaytime);                  //�ӳٵȴ�
	cout << "д���߳�" << num_thread << "����д�ļ�." << endl;
	WaitForSingleObject(writeCountSignal, -1);//��writercount�������,����д�߼�������Դ
	if (writercount == 0)
		WaitForSingleObject(read_s, -1);
	//��һλд��������Դ
	writercount++;
	ReleaseSemaphore(writeCountSignal, 1, NULL);//�ͷ�д�߼�������Դ
	WaitForSingleObject(wrt, -1);//�����ļ���Դ
	/*write is performed*/
	cout << "д���߳�" << num_thread << "��ʼд�ļ�." << endl;
	Sleep(duration);
	cout << "д���߳�" << num_thread << "���д�ļ�." << endl;
	ReleaseSemaphore(wrt, 1, NULL);//�ͷ���Դ
	WaitForSingleObject(writeCountSignal, -1);//��writercount������ʣ������д�߻���
	writercount--;
	if (writercount == 0)
		ReleaseSemaphore(read_s, 1, NULL);
	//�ͷ���Դ
	ReleaseSemaphore(writeCountSignal, 1, NULL);//�ͷ���Դ
}
//д������
void WriterPriority() {
	DWORD n_thread = 0;           //�߳���Ŀ
	DWORD thread_ID;            //�߳�ID
	DWORD wait_for_all;         //�ȴ������߳̽���
								//�����ź���
	RCSignal = CreateSemaphore(NULL, 1, 1, _T("mutex_for_readercount"));//���߶�count�޸Ļ����ź�������ֵΪ1,���Ϊ1
	writeCountSignal = CreateSemaphore(NULL, 1, 1, _T("mutex_for_writercount"));//д�߶�count�޸Ļ����ź�������ֵΪ1,���Ϊ1
	wrt = CreateSemaphore(NULL, 1, 1, NULL);//
	read_s = CreateSemaphore(NULL, 1, 1, NULL);//�鼮��������ź�������ֵΪ1,���ֵΪ1
	HANDLE h_Thread[MAX_THREAD_NUM];//�߳̾��,�̶߳��������
	thread_info thread_info[MAX_THREAD_NUM];//�����߳���Ϣ������
	int id = 0;
	readercount = 0;               //��ʼ��readcount
	writercount = 0;
	cout << "д������:" << endl;
	fstream file;
	file.open("����.txt", ios::in);
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
			//�������߽���
			h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(wp_threadReader), &thread_info[i], 0, &thread_ID);
		}
		else
		{
			//����д�߳�
			h_Thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(wp_threadWriter), &thread_info[i], 0, &thread_ID);
		}
	}
	//�ȴ����߳̽���
	//�رվ��
	wait_for_all = WaitForMultipleObjects(n_thread, h_Thread, TRUE, -1);
	cout << endl;
	cout << "���ж���д���Ѿ���ɲ�������" << endl;
	for (int i = 0; i < (int)(n_thread); i++)
		CloseHandle(h_Thread[i]);
	CloseHandle(writeCountSignal);
	CloseHandle(RCSignal);
	CloseHandle(wrt);
}
//������
int main()
{
	char choice;
	//cout << "    ��д�������Գ���    " << endl;
	cout << endl;
	while (true)
	{
		//��ӡ��ʾ��Ϣ
		cout << "�밴������������ţ�       " << endl;
		cout << "1����������" << endl;
		cout << "2��д������" << endl;
		cout << "3���˳�����" << endl;
		cout << endl;
		//���������Ϣ����ȷ����������
		do {
			choice = (char)_getch();
		} while (choice != '1' && choice != '2' && choice != '3');
		system("cls");//����
		//ѡ��1����������
		if (choice == '1') {
			//ReaderPriority("thread.txt");
			ReaderPriority();
			system("pause");
		}
		//ѡ��2��д������
		else if (choice == '2') {
			WriterPriority();
			system("pause");
		}
		//ѡ��3���˳�
		else
			return 0;
		//����
		cout << "\nPress Any Key to Coutinue";
		_getch();
		system("cls");
	}
	return 0;
}