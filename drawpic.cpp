// drawpic.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include<iostream>
#include<afxglobals.h>
#include<OleCtl.h>
#include<OCIdl.h>
#include<time.h>
#include<atlimage.h>
#include<thread>
#include<condition_variable>
#include<queue>

#include<mutex>
#include<Mmsystem.h>

#pragma comment(lib, "winmm.lib")

using namespace std;





class Semaphore {
public:
	Semaphore(int count_ = 0)
		: count(count_) {}

	inline void up()
	{
		std::unique_lock<std::mutex> lock(mtx);
		count++;
		cv.notify_one();
	}

	inline void down()
	{
		std::unique_lock<std::mutex> lock(mtx);

		while (count == 0) {
			cv.wait(lock);
		}
		count--;
	}

private:
	std::mutex mtx;
	std::condition_variable cv;
	int count;
};



Semaphore mux[3] = {1 ,0 ,0}, m_empty = 0,m_full=100;//同步锁
mutex m_lock;//互斥锁

CImage* Qt[101];
int q_begin = 0, q_end = 0;






class FrameTimer //计时器
{
private:
	unsigned int flag;
public:
	void start()
	{
		flag = clock();
	}
	operator int()
	{
		return ((clock() - flag));
	}

};

HWND GetConsoleHwnd() {
	HWND hwndFound;
	char Name[256];
	GetConsoleTitle((LPWSTR)Name, 256);
	hwndFound = FindWindow(NULL, (LPWSTR)Name);
	return(hwndFound);
}
HWND GetCPUGraphHWND(HWND father,HWND child,int size=100)
{
	RECT rect;
	int times = 500;
	while (times) 
	{
		HWND res = FindWindowEx(child, 0, (LPWSTR)L"CvChartWindow", NULL);
		if (res)
		{
			GetWindowRect(res, &rect);
			if ((rect.right - rect.left) > size)
			{
				return res;
			}
		}

		child= FindWindowEx(father, child, (LPWSTR)L"CtrlNotifySink", NULL);
		
		times--;
	}
	return NULL;
}


HWND GetTSMHwnd() {
	HWND hwndFound,hwndChild;
	hwndFound = FindWindow(NULL, (LPWSTR)L"任务管理器");//FindWindow只能获取顶级窗口 
	hwndFound=FindWindowEx(hwndFound, 0,  NULL, (LPWSTR)L"TaskManagerMain");
	hwndFound = FindWindowEx(hwndFound, 0, (LPWSTR)L"DirectUIHWND",NULL);
	hwndChild = FindWindowEx(hwndFound, 0, (LPWSTR)L"CtrlNotifySink", NULL);
	hwndFound = GetCPUGraphHWND(hwndFound, hwndChild, 100);
	
	return(hwndFound);
}

wchar_t* c2w(const char *str)
{
	int length = strlen(str) + 1;
	wchar_t *t = (wchar_t*)malloc(sizeof(wchar_t)*length);
	memset(t, 0, length * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str, strlen(str), t, length);
	return t;
}

void ImageChange(CImage *img,int w,int h, int ft)
{
	int UpPixel = RGB(0, 0, 0);
	byte* pRealData;
	pRealData = (byte*)img->GetBits();
	int pit = img->GetPitch();
	int bitCount = img->GetBPP() / 8;
	for (int x = 0; x < img->GetWidth(); x++)   //边缘检测
	{
		for (int y = 0; y < img->GetHeight(); y++)
		{

			//int grayVal = (int)(int)(*(pRealData + pit * y + x * bitCount)) > 192 ? 255 : 0;
			int grayVal = (int)(int)(*(pRealData + pit * y + x * bitCount));
			if (abs(grayVal - UpPixel) > 10)
			{
				*(pRealData + pit * y + x * bitCount) = 187;
				*(pRealData + pit * y + x * bitCount + 1) = 152;
				*(pRealData + pit * y + x * bitCount + 2) = 17;

				//*(pRealData + pit * y + x * bitCount) = 180;
				//*(pRealData + pit * y + x * bitCount + 1) = 40;
				//*(pRealData + pit * y + x * bitCount + 2) = 149;

				//*(pRealData + pit * y + x * bitCount) = 1;
				//*(pRealData + pit * y + x * bitCount + 1) = 79;
				//*(pRealData + pit * y + x * bitCount + 2) = 167;
			}
			UpPixel = grayVal;
		}
	}
	for (int y = 0; y < img->GetHeight(); y++)   //边缘检测
	{
		for (int x = 0; x < img->GetWidth(); x++)
		{

			//int grayVal = (int)(int)(*(pRealData + pit * y + x * bitCount)) > 192 ? 255 : 0;
			int grayVal = (int)(int)(*(pRealData + pit * y + x * bitCount));
			if (abs(grayVal - UpPixel) > 10)
			{
				*(pRealData + pit * y + x * bitCount) = 187;       //B
				*(pRealData + pit * y + x * bitCount + 1) = 152;    //G
				*(pRealData + pit * y + x * bitCount + 2) = 17;     //R

				//*(pRealData + pit * y + x * bitCount) = 1;
				//*(pRealData + pit * y + x * bitCount + 1) = 79;
				//*(pRealData + pit * y + x * bitCount + 2) = 167;
			}
			if ((x == 0 || x == img->GetWidth() - 1) || (y == 0 || y == img->GetHeight() - 1))  //深蓝色边框
			{
				*(pRealData + pit * y + x * bitCount) = 187;       //B
				*(pRealData + pit * y + x * bitCount + 1) = 152;    //G
				*(pRealData + pit * y + x * bitCount + 2) = 17;     //R

				//*(pRealData + pit * y + x * bitCount) = 1;
				//*(pRealData + pit * y + x * bitCount + 1) = 79;
				//*(pRealData + pit * y + x * bitCount + 2) = 167;
			}
			UpPixel = grayVal;
		}
	}
	for (int y = 0; y < img->GetHeight(); y++)  //背景处理
	{
		for (int x = 0; x < img->GetWidth(); x++)
		{

			int grayVal = (int)(int)(*(pRealData + pit * y + x * bitCount));
			if (grayVal < 186 )       //背景设为浅蓝色
			{
				//if ((int)(int)(*(pRealData + pit * y + x * bitCount)) == 1
				//	&& (int)(int)(*(pRealData + pit * y + x * bitCount+1)) == 79)
				//	continue;
				*(pRealData + pit * y + x * bitCount) = 250;       //B
				*(pRealData + pit * y + x * bitCount + 1) = 246;    //G
				*(pRealData + pit * y + x * bitCount + 2) = 241;     //R

				//*(pRealData + pit * y + x * bitCount) = 244;       //B
				//*(pRealData + pit * y + x * bitCount + 1) = 242;    //G
				//*(pRealData + pit * y + x * bitCount + 2) = 244;     //R

				//*(pRealData + pit * y + x * bitCount) = 235;       //B
				//*(pRealData + pit * y + x * bitCount + 1) = 243;    //G
				//*(pRealData + pit * y + x * bitCount + 2) = 252;     //R

			}
			if ((y % (h / 8) == 0) && y != 0)   //画方格的横线
			{
				*(pRealData + pit * y + x * bitCount) = 240;       //B    
				*(pRealData + pit * y + x * bitCount + 1) = 226;    //G
				*(pRealData + pit * y + x * bitCount + 2) = 206;    //R

				//*(pRealData + pit * y + x * bitCount) = 240;       //B
				//*(pRealData + pit * y + x * bitCount + 1) = 222;    //G
				//*(pRealData + pit * y + x * bitCount + 2) = 236;     //R

				//*(pRealData + pit * y + x * bitCount) = 207;       //B
				//*(pRealData + pit * y + x * bitCount + 1) = 222;    //G
				//*(pRealData + pit * y + x * bitCount + 2) = 238;     //R
			}
			//if ((x % (w / 5) == (ft/1000 )*((w / 40)) % (w / 20)) && x != 0)   //画方格的竖线
			if ((x % (w / 5)) == ((ft / 1000)*(w/6))%(w/5) && x != 0)
			{
				int rx = x;
				*(pRealData + pit * y + rx * bitCount) = 240;       //B
				*(pRealData + pit * y + rx * bitCount + 1) = 226;    //G
				*(pRealData + pit * y + rx * bitCount + 2) = 206;    //R

				//*(pRealData + pit * y + x * bitCount) = 240;       //B
				//*(pRealData + pit * y + x * bitCount + 1) = 222;    //G
				//*(pRealData + pit * y + x * bitCount + 2) = 236;     //R

				//*(pRealData + pit * y + x * bitCount) = 207;       //B
				//*(pRealData + pit * y + x * bitCount + 1) = 222;    //G
				//*(pRealData + pit * y + x * bitCount + 2) = 238;     //R
			}
		}

	}
}

void ImageImage( int w, int h,int num,int mod ,int istart,int fullsize)
{
	char buffer[256];
	int i = istart;
	
	while (i++ < fullsize) {
		if ((i-1)%num == mod)
		{
			sprintf_s(buffer, ".\\bailan\\bailan%04d.jpg", i);
			CImage *img = new CImage();

			img->Load((LPCTSTR)c2w(buffer));

			ImageChange(img, w, h, i*33);
			
			mux[mod].down();
			m_empty.up();
			m_full.down();
			
			m_lock.lock();
			Qt[(q_end++) % 101] = img;
			//printf("%d\n", i);
			m_lock.unlock();

			mux[(mod + 1) % num].up();
		}
	}


}



void DisplayImageCImage(HWND hwnd,int w,int h)
{
	HDC hdc = GetDC(hwnd);

	m_empty.down();
	m_full.up();
	m_lock.lock();

	CImage *img = Qt[(q_begin++)%101];
	
	m_lock.unlock();
	//alock.unlock();
	double resize = 1.25;
	img->Draw(hdc, 0, 0, (int)w*resize, (int)h*resize);
	img->Destroy();

}



int main()
{
	int fullsize = 1765;
	int istart = 0;
	
	
	FrameTimer ft;
	RECT rect;
	HWND hwnd = GetTSMHwnd();
	GetWindowRect(hwnd, &rect);
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top;
	
	thread thread1(ImageImage, w, h, 3, 0, istart,fullsize), thread2(ImageImage, w, h, 3, 1, istart, fullsize),thread3(ImageImage, w, h, 3, 2, istart, fullsize);
	thread1.detach();
	thread2.detach();
	thread3.detach();
	int i = 0;
	PlaySound(L"bailan.wav", NULL, SND_FILENAME | SND_ASYNC);
	while (i++ < fullsize)
	{	
		ft.start();
	/*	hwnd = GetTSMHwnd();*/
		GetWindowRect(hwnd, &rect);
		w = rect.right - rect.left;
		h = rect.bottom - rect.top;
		DisplayImageCImage(hwnd,w,h);
	
		for (;(int)ft < 42;);
	}

}


