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
//void DisplayImage(HWND hwnd, LPCTSTR szImagePath)
//{
//	HDC hDC = GetDC(hwnd);
//	RECT rect;
//	GetWindowRect(hwnd, &rect);
//	HANDLE hFile = CreateFile(szImagePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL); //从指定的路径szImagePath中读取文件句柄
//	if ((int)hFile == -1)
//	{
//		printf("File load error!\n");
//		exit(-1);
//	}
//	DWORD dwFileSize = GetFileSize(hFile, NULL); //获得图片文件的大小，用来分配全局内存
//	HGLOBAL hImageMemory = GlobalAlloc(GMEM_MOVEABLE, dwFileSize); //给图片分配全局内存
//	void *pImageMemory = GlobalLock(hImageMemory); //锁定内存
//	DWORD dwReadedSize; //保存实际读取的文件大小
//	ReadFile(hFile, pImageMemory, dwFileSize, &dwReadedSize, NULL); //读取图片到全局内存当中
//	GlobalUnlock(hImageMemory); //解锁内存
//	CloseHandle(hFile); //关闭文件句柄
//	IStream *pIStream;//创建一个IStream接口指针，用来保存图片流
//	IPicture *pIPicture;//创建一个IPicture接口指针，表示图片对象
//	CreateStreamOnHGlobal(hImageMemory, false, &pIStream); //用全局内存初使化IStream接口指针
//	HRESULT ea=OleLoadPicture(pIStream, 0, false, IID_IPicture, (LPVOID*)&(pIPicture));//用OleLoadPicture获得IPicture接口指针
//																			//得到IPicture COM接口对象后，你就可以进行获得图片信息、显示图片等操作
//	OLE_XSIZE_HIMETRIC hmWidth;
//	OLE_YSIZE_HIMETRIC hmHeight;
//
//	pIPicture->get_Width(&hmWidth); //用接口方法获得图片的宽和高
//	pIPicture->get_Height(&hmHeight);
//	//hmHeight = rect.bottom - rect.top;
//	//hmWidth = rect.right - rect.left;
//	double resize = 1.25;
//	pIPicture->Render(hDC, 0, 0, (rect.right - rect.left)*resize , (rect.bottom - rect.top)*resize, 0, hmHeight, hmWidth, -hmHeight, NULL); //在指定的DC上绘出图片
//	//pIPicture->Render(hDC, 0, 0, hmWidth / 25, hmHeight / 25, 0, hmHeight, hmWidth, -hmHeight, NULL); //在指定的DC上绘出图片
//	pIPicture->PictureChanged();
//	GlobalFree(hImageMemory); //释放全局内存
//	pIStream->Release(); //释放pIStream
//	pIPicture->Release(); //释放pIPicture
//
//}
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


//int main(void)
//{
//	float center1X = 200;
//	float center1Y = 100;
//	float center2X = 350;
//	float center2Y = 250;
//	float center0X = (center1X + center2X) / 2;
//	float center0Y = (center1Y + center2Y) / 2;
//	float centerRradius = (center2X - center1X) / 2;
//
//	float around1X = 0;
//	float around1Y = 0;
//	float around2X = 0;
//	float around2Y = 0;
//	float around0X = 0;
//	float around0Y = 0;
//	float aroundRradius = 20;
//	int angle = 0;
//	//HWND hConsole = GetConsoleHwnd();    // 获得控制台窗口句柄
//	HDC hDC = GetDC((HWND)0x000107C2);    // 获得控制台窗口绘图DC
//
//	HPEN hPen = CreatePen(0, 5, RGB(255, 255, 255));
//	HBRUSH hBrush1 = CreateSolidBrush(RGB(81, 24, 214));
//	HBRUSH hBrush2 = CreateSolidBrush(RGB(169, 138, 0));
//	HBRUSH hBrush3 = CreateSolidBrush(RGB(0, 0, 0));
//
//	HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);    // 让DC选择此画笔
//	HDC pdc = GetDC((HWND)0x000107C2);
//
//	hOldPen = (HPEN)SelectObject(pdc, hBrush1);
//	Ellipse(pdc, center1X, center1Y, center2X, center2Y);//用笔刷画实心圆 ;
//	while (1)
//	{
//		hOldPen = (HPEN)SelectObject(pdc, hBrush2);
//		around0X = center0X - (centerRradius + 40)*sin((angle % 360) / 180.0 * 3.14159265358979);
//		around0Y = center0Y + (centerRradius + 40)*cos((angle % 360) / 180.0 * 3.14159265358979);
//		Ellipse(pdc, around0X - aroundRradius, around0Y + aroundRradius, around0X + aroundRradius, around0Y - aroundRradius);
//		Sleep(10);
//		hOldPen = (HPEN)SelectObject(pdc, hBrush3);
//		Ellipse(pdc, around0X - aroundRradius, around0Y + aroundRradius, around0X + aroundRradius, around0Y - aroundRradius);
//		angle++;
//	}
//
//
//	//Ellipse(pdc,5,5,45,45);//用笔刷画实心圆 ;
//	//MoveToEx(hDC,0, 250, NULL);
//	//Arc(hDC,100,100,300,300,350,500,350,500);//用画笔画空心圆 ; 
//	//for(int i=0;i<500;i++)
//	//   LineTo(hDC,10*i,150+100*sin(i*6));
//	//InvalidateRect(hConsole, NULL, TRUE); // 刷新窗口
//
//	ReleaseDC((HWND)0x000107C2, hDC);// 释放DC
//	while (1);
//	return 0;
//}