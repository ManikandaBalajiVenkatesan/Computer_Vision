
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <process.h>	/* needed for multithreading */
#include "resource.h"
#include "globals.h"

#define MAX_QUEUE 10000	/* max perimeter size (pixels) of border wavefront */

#define SQR(x) ((x)*(x))

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPTSTR lpCmdLine, int nCmdShow)

{
	MSG			msg;
	HWND		hWnd;
	WNDCLASS	wc;

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, "ID_PLUS_ICON");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = "ID_MAIN_MENU";
	wc.lpszClassName = "PLUS";

	if (!RegisterClass(&wc))
		return(FALSE);

	hWnd = CreateWindow("PLUS", "plus program",
		WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
		CW_USEDEFAULT, 0, 400, 400, NULL, NULL, hInstance, NULL);
	if (!hWnd)
		return(FALSE);

	ShowScrollBar(hWnd, SB_BOTH, FALSE);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	MainWnd = hWnd;

	ShowPixelCoords = 0;

	strcpy(filename, "");
	OriginalImage = NULL;
	ROWS = COLS = 0;

	x_mouse = 0; y_mouse = 0;
	ThreadRegion = 0;
	RegionNumber = 0;
	SegColor = RGB(255, 0, 0);
	

	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return(msg.wParam);
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam)

{
	HMENU				hMenu;
	OPENFILENAME		ofn;
	FILE* fpt;
	HDC					hDC;
	char				header[320], text[320];
	int					BYTES, xPos, yPos;

	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_SHOWPIXELCOORDS:
			ShowPixelCoords = (ShowPixelCoords + 1) % 2;
			PaintImage();
			break;

		case ID_SETTINGS:
			SettingsDialogBox();
	
		case ID_DISPLAY_UNDOREGION:
			//for (int i = 0; i < ROWS * COLS; i++)
			//	OriginalImage[i] = ImageCopy[i];
			PaintImage();
			break;

		case ID_GROWMODE_PLAY:
			PlayMode = (PlayMode + 1) % 2;
			if (PlayMode == 1)
				_beginthread(RegionGrowPlay, 0, MainWnd);
			break;

		case ID_GROWMODE_STEP:
			StepMode = (StepMode + 1) % 2;
			break;

		case ID_COLOR_GREEN:
			SegColor = RGB(0, 255, 0);
			break;

		case ID_COLOR_RED:
			SegColor = RGB(255, 0, 0);
			break;

		case ID_COLOR_BLACK:
			SegColor = RGB(0, 0, 0);
			break;

		case ID_COLOR_YELLOW:
			SegColor = RGB(255, 255, 0);
			break;

		case ID_COLOR_BLUE:
			SegColor = RGB(0, 0, 255);
			break;

		case ID_FILE_LOAD:
			if (OriginalImage != NULL)
			{
				free(OriginalImage);
				OriginalImage = NULL;
			}
			memset(&(ofn), 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFile = filename;
			filename[0] = 0;
			ofn.nMaxFile = MAX_FILENAME_CHARS;
			ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
			ofn.lpstrFilter = "PPM files\0*.ppm\0All files\0*.*\0\0";
			if (!(GetOpenFileName(&ofn)) || filename[0] == '\0')
				break;		/* user cancelled load */
			if ((fpt = fopen(filename, "rb")) == NULL)
			{
				MessageBox(NULL, "Unable to open file", filename, MB_OK | MB_APPLMODAL);
				break;
			}
			fscanf(fpt, "%s %d %d %d", header, &COLS, &ROWS, &BYTES);
			if (strcmp(header, "P5") != 0 || BYTES != 255)
			{
				MessageBox(NULL, "Not a PPM (P5 greyscale) image", filename, MB_OK | MB_APPLMODAL);
				fclose(fpt);
				break;
			}
			OriginalImage = (unsigned char*)calloc(ROWS * COLS, 1);
			OriginalImageRGB = (unsigned char*)calloc(ROWS * COLS * 3, 1);
			ImageCopy = (unsigned char*)calloc(ROWS * COLS, 1);
			label = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
			header[0] = fgetc(fpt);	/* whitespace character after header */
			fread(OriginalImage, 1, ROWS * COLS, fpt);
			fclose(fpt);
			for (int i = 0; i < ROWS * COLS; i++)
			{
				ImageCopy[i] = OriginalImage[i];
				OriginalImageRGB[i * 3 + 0] = (OriginalImage[i] * 53) % 255;
				OriginalImageRGB[i * 3 + 1] = (OriginalImage[i] * 97) % 255;
				OriginalImageRGB[i * 3 + 2] = (OriginalImage[i] * 233) % 255;

			}
				
			SetWindowText(hWnd, filename);
			PaintImage();
			break;

		case ID_FILE_QUIT:
			DestroyWindow(hWnd);
			break;
		}
		break;
	case WM_SIZE:		  /* could be used to detect when window size changes */
		PaintImage();
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_PAINT:
		PaintImage();
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_LBUTTONDOWN:
		x_mouse = LOWORD(lParam);
		y_mouse = HIWORD(lParam);
		sprintf(text, "mouse click %d,%d     ", x_mouse, y_mouse);
		hDC = GetDC(MainWnd);
		TextOut(hDC, 0, 0, text, strlen(text));
		ReleaseDC(MainWnd, hDC);
		ThreadRegion = 1;
		RegionNumber++;
		_beginthread(RegionGrowThread, 0, MainWnd);

		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;


	case WM_RBUTTONDOWN:
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_MOUSEMOVE:
		if (ShowPixelCoords == 1)
		{
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
			if (xPos >= 0 && xPos < COLS && yPos >= 0 && yPos < ROWS)
			{
				sprintf(text, "%d,%d=>%d     ", xPos, yPos, OriginalImage[yPos * COLS + xPos]);
				hDC = GetDC(MainWnd);
				TextOut(hDC, 0, 0, text, strlen(text));		/* draw text on the window */
				SetPixel(hDC, xPos, yPos, RGB(255, 0, 0));	/* color the cursor position red */
				ReleaseDC(MainWnd, hDC);
			}
		}
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_KEYDOWN:
		if (wParam == 's' || wParam == 'S')
			PostMessage(MainWnd, WM_COMMAND, ID_SHOWPIXELCOORDS, 0);	  /* send message to self */
		
		if (wParam == 'j' || wParam == 'J')
		{
			if (StepMode == 1 && SegDone == 1)
			{
				RegionGrowStep(index);

				index++;
			}
		}

		if ((TCHAR)wParam == '1')
		{
			TimerRow = TimerCol = 0;
			SetTimer(MainWnd, TIMER_SECOND, 10, NULL);	/* start up 10 ms timer */
		}
		if ((TCHAR)wParam == '2')
		{
			KillTimer(MainWnd, TIMER_SECOND);			/* halt timer, stopping generation of WM_TIME events */
			PaintImage();								/* redraw original image, erasing animation */
		}
		if ((TCHAR)wParam == '3')
		{
			ThreadRunning = 1;
			_beginthread(AnimationThread, 0, MainWnd);	/* start up a child thread to do other work while this thread continues GUI */
		}
		if ((TCHAR)wParam == '4')
		{
			ThreadRunning = 0;							/* this is used to stop the child thread (see its code below) */
		}
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_TIMER:	  /* this event gets triggered every time the timer goes off */
		hDC = GetDC(MainWnd);
		SetPixel(hDC, TimerCol, TimerRow, RGB(0, 0, 255));	/* color the animation pixel blue */
		ReleaseDC(MainWnd, hDC);
		TimerRow++;
		TimerCol += 2;
		break;
	case WM_HSCROLL:	  /* this event could be used to change what part of the image to draw */
		PaintImage();	  /* direct PaintImage calls eliminate flicker; the alternative is InvalidateRect(hWnd,NULL,TRUE); UpdateWindow(hWnd); */
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_VSCROLL:	  /* this event could be used to change what part of the image to draw */
		PaintImage();
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	}

	hMenu = GetMenu(MainWnd);
	if (ShowPixelCoords == 1)
		CheckMenuItem(hMenu, ID_SHOWPIXELCOORDS, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_SHOWPIXELCOORDS, MF_UNCHECKED);

	if (PlayMode == 1)
		CheckMenuItem(hMenu, ID_GROWMODE_PLAY, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_GROWMODE_PLAY, MF_UNCHECKED);

	if (StepMode == 1)
		CheckMenuItem(hMenu, ID_GROWMODE_STEP, MF_CHECKED);	/* you can also call EnableMenuItem() to grey(disable) an option */
	else
		CheckMenuItem(hMenu, ID_GROWMODE_STEP, MF_UNCHECKED);

	DrawMenuBar(hWnd);

	return(0L);
}




void PaintImage()

{
	PAINTSTRUCT			Painter;
	HDC					hDC;
	BITMAPINFOHEADER	bm_info_header;
	BITMAPINFO* bm_info;
	int					i, r, c, DISPLAY_ROWS, DISPLAY_COLS;
	unsigned char* DisplayImage;

	if (OriginalImage == NULL)
		return;		/* no image to draw */

			  /* Windows pads to 4-byte boundaries.  We have to round the size up to 4 in each dimension, filling with black. */
	DISPLAY_ROWS = ROWS;
	DISPLAY_COLS = COLS;
	if (DISPLAY_ROWS % 4 != 0)
		DISPLAY_ROWS = (DISPLAY_ROWS / 4 + 1) * 4;
	if (DISPLAY_COLS % 4 != 0)
		DISPLAY_COLS = (DISPLAY_COLS / 4 + 1) * 4;
	DisplayImage = (unsigned char*)calloc(DISPLAY_ROWS * DISPLAY_COLS, 1);
	for (r = 0; r < ROWS; r++)
		for (c = 0; c < COLS; c++)
			DisplayImage[r * DISPLAY_COLS + c] = OriginalImage[r * COLS + c];

	BeginPaint(MainWnd, &Painter);
	hDC = GetDC(MainWnd);
	bm_info_header.biSize = sizeof(BITMAPINFOHEADER);
	bm_info_header.biWidth = DISPLAY_COLS;
	bm_info_header.biHeight = -DISPLAY_ROWS;
	bm_info_header.biPlanes = 1;
	bm_info_header.biBitCount = 8;
	bm_info_header.biCompression = BI_RGB;
	bm_info_header.biSizeImage = 0;
	bm_info_header.biXPelsPerMeter = 0;
	bm_info_header.biYPelsPerMeter = 0;
	bm_info_header.biClrUsed = 256;
	bm_info_header.biClrImportant = 256;
	bm_info = (BITMAPINFO*)calloc(1, sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD));
	bm_info->bmiHeader = bm_info_header;
	for (i = 0; i < 256; i++)
	{
		bm_info->bmiColors[i].rgbBlue = bm_info->bmiColors[i].rgbGreen = bm_info->bmiColors[i].rgbRed = i;
		bm_info->bmiColors[i].rgbReserved = 0;
	}

	SetDIBitsToDevice(hDC, 0, 0, DISPLAY_COLS, DISPLAY_ROWS, 0, 0,
		0, /* first scan line */
		DISPLAY_ROWS, /* number of scan lines */
		DisplayImage, bm_info, DIB_RGB_COLORS);
	ReleaseDC(MainWnd, hDC);
	EndPaint(MainWnd, &Painter);

	free(DisplayImage);
	free(bm_info);
}


void RegionGrowStep(int id)
{
	HDC hDC;
	hDC = GetDC(MainWnd);
	int r = indices[id] / COLS;
	int c = indices[id] % COLS;
	SetPixel(hDC, c, r, SegColor);	
	ReleaseDC(MainWnd, hDC);
	//OriginalImage[indices[index]] = SegColor;
}

void RegionGrowPlay(HWND RegionGrowHandle)
{
	HDC hDC;
	int* indices_local = (int*)calloc(ROWS * COLS, sizeof(int));
	
	while (PlayMode == 1)
	{
		if (SegDone == 1)
		{
			while(index < count)
			{
				//OriginalImage[indices[i]] = 0;

				hDC = GetDC(MainWnd);
				int r = indices[index] / COLS;
				int c = indices[index] % COLS;
				SetPixel(hDC, c, r, SegColor);	/* color the cursor position red */
				ReleaseDC(MainWnd, hDC);
				Sleep(1);
				index++;
				if (PlayMode == 0)
					break;
			}
		}
	
	}
	//PaintImage();
}

void RegionGrowThread(HWND RegionGrowHandle)
{
	HDC hDC;
	char text[300];
	int r = y_mouse, c = x_mouse;
	int region_id = RegionNumber;
	double avg = 0.0, var = 0.0;
	int	r2, c2;
	int	queue[MAX_QUEUE], qh, qt;
	int	average, total;		/* average and total intensity in growing region */
	count = 0;		//number of pixels joined in the zone
	indices = (int*)calloc(ROWS * COLS, sizeof(int));
	SegDone = 0;		//flag that indicates region growth has started
	index = 0;

	//calculating the variance of 7x7 box to see whether a region can be grown
	for (int r2 = y_mouse - 3; r2 <= y_mouse + 3; r2++)
		for (int c2 = x_mouse - 3; c2 <= x_mouse + 3; c2++)
			avg += (double)(OriginalImage[(r2)*COLS + (c2)]);
	avg = avg / 49.0;
	for (int r2 = y_mouse - 3; r2 <= y_mouse + 3; r2++)
		for (int c2 = x_mouse - 3; c2 <= x_mouse + 3; c2++)
			var += SQR(avg - (double)OriginalImage[(r2)*COLS + (c2)]);
	var = sqrt(var) / 49.0;
	//sprintf(text, "average %lf variance %lf     ", avg, var);
	//hDC = GetDC(MainWnd);
	//TextOut(hDC, 50, 50, text, strlen(text));		/* draw text on the window */
	//ReleaseDC(MainWnd, hDC);



	//checking whether the current pixel can have any potential region around it
	if (var > 1.0)
		return;



	//sprintf(text, "zone started  %d  %d  ", region_id, label[r * COLS + c]);
	//hDC = GetDC(MainWnd);
	//TextOut(hDC, 70, 70, text, strlen(text));		/* draw text on the window */
	//ReleaseDC(MainWnd, hDC);

	
	label[r * COLS + c] = region_id;	//setting the region id of that pixel
	

	average = total = (int)OriginalImage[r * COLS + c];

	queue[0] = r * COLS + c;
	qh = 1;	// queue head 
	qt = 0;	// queue tail 
	indices[count] = r * COLS + c;
	count++;



	while (qt != qh)
	{


		if (count % 50 == 0)	// recalculate average after each 50 pixels join 
			average = total / count;

		for (r2 = -1; r2 <= 1; r2++)
			for (c2 = -1; c2 <= 1; c2++)
			{
				if (r2 == 0 && c2 == 0)		//no need to check the same pixel again
					continue;
				if ((queue[qt] / COLS + r2) < 0 || (queue[qt] / COLS + r2) >= ROWS ||
					(queue[qt] % COLS + c2) < 0 || (queue[qt] % COLS + c2) >= COLS)			//checking whether it is out of image
					continue;
				if (label[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2] != 0)	//don't merge if it is already part of another region
					continue;
				//test criteria to join region - whether the pixel value exceeds average of the region by 50
				if (abs((int)(OriginalImage[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2]) - average) > AvgDiff)
					continue;
				label[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2] = region_id;	//the pixel has passed the test and added to the region
				total += OriginalImage[(queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2];	//calculating the sum of pixel values of the region
				indices[count] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
				count++;	//incrementing the number of pixels in the region
				queue[qh] = (queue[qt] / COLS + r2) * COLS + queue[qt] % COLS + c2;
				qh = (qh + 1) % MAX_QUEUE;
				if (qh == qt)
				{
					printf("Max queue size exceeded\n");
					return;
				}
			}
		qt = (qt + 1) % MAX_QUEUE;

	}

	SegDone = 1;		//region growth has been completed
	
	/*
	for (int i = 0; i < count; i++)
		if (label[indices[i]] == region_id)
		{
			OriginalImage[indices[i]] = 0;
			Sleep(1);
			PaintImage();
		}
		*/	

	/*
	if (count != 0)
		for (int i = 0; i < ROWS * COLS; i++)
			if (label[i] == region_id)
				OriginalImage[i] = 0;
				*/
	//PaintImage();


}


void SettingsDialogBox()
{
	MessageBox(NULL, "qwer", "ASDF", NULL);

}

void AnimationThread(HWND AnimationWindowHandle)

{
	HDC		hDC;
	char	text[300];

	ThreadRow = ThreadCol = 0;
	while (ThreadRunning == 1)
	{
		hDC = GetDC(MainWnd);
		SetPixel(hDC, ThreadCol, ThreadRow, RGB(0, 255, 0));	/* color the animation pixel green */
		sprintf(text, "%d,%d     ", ThreadRow, ThreadCol);
		TextOut(hDC, 300, 0, text, strlen(text));		/* draw text on the window */
		ReleaseDC(MainWnd, hDC);
		ThreadRow += 3;
		ThreadCol++;
		Sleep(100);		/* pause 100 ms */
	}
}

