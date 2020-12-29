
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

int l_click_move = 0;

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
			copy_image(ImageCopy, OriginalImage);

			for (int i = 0; i < MAX_PIXEL; i++)
			{
				x_pixel[i] = 0; y_pixel[i] = 0;
				if (i < MAX_PIXEL / SAMPLE_SIZE)
				{
					px[i] = 0; py[i] = 0;
				}
			}
			PaintImage();
			break;
		
		case ID_DISPLAY_SOBELEDGE:
			sobel_edge();
			copy_image(sobel_image, OriginalImage);
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
			ofn.lpstrFilter = "ALL files\0*.*\0PNM files\0*.pnm\0PPM files\0*.ppm\0All files\0*.*\0\0";
			if (!(GetOpenFileName(&ofn)) || filename[0] == '\0')
				break;		/* user cancelled load */
			if ((fpt = fopen(filename, "rb")) == NULL)
			{
				MessageBox(NULL, "Unable to open file", filename, MB_OK | MB_APPLMODAL);
				break;
			}
			fscanf(fpt, "%s %d %d %d ", header, &COLS, &ROWS, &BYTES);

			if ((strcmp(header, "P6") != 0 && strcmp(header, "P5") != 0) || BYTES != 255)
			{
				MessageBox(NULL, "Not a PNM (P6) or PPM (P5) image", filename, MB_OK | MB_APPLMODAL);
				fclose(fpt);
				break;
			}

	

			OriginalImage = (unsigned char*)calloc(ROWS * COLS, 1);
			OriginalImageRGB = (unsigned char*)calloc(ROWS * COLS * 3, 1);
			
			ImageCopy = (unsigned char*)calloc(ROWS * COLS, 1);
			label = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
			sobel_image = (unsigned char*)calloc(ROWS * COLS, sizeof(unsigned char));
			edge_image = (double*)calloc(ROWS * COLS, sizeof(double));
			temp = (unsigned char*)calloc((ROWS + 2) * (COLS + 2), sizeof(unsigned char));

			
			//header[0] = fgetc(fpt);	/* whitespace character after header */
			if (strcmp(header, "P6") == 0)
				fread(OriginalImageRGB, 1, ROWS * COLS * 3, fpt); //loading color image
							
			if (strcmp(header, "P5") == 0)
				fread(OriginalImage, 1, ROWS * COLS, fpt);
	
				
			
			fclose(fpt);
			for (int i = 0; i < ROWS * COLS; i++)
			{
				//greyscale to rgb
				if (strcmp(header, "P5") == 0)
				{
					OriginalImageRGB[i * 3 + 0] = (OriginalImage[i] * 53) % 255;
					OriginalImageRGB[i * 3 + 1] = (OriginalImage[i] * 97) % 255;
					OriginalImageRGB[i * 3 + 2] = (OriginalImage[i] * 233) % 255;
					
				}
					
				//rgb to greyscale
				if (strcmp(header, "P6") == 0)
					OriginalImage[i] = (OriginalImageRGB[i * 3 + 0] + OriginalImageRGB[i * 3 + 1] + OriginalImageRGB[i * 3 + 2]) / 3;

					
				ImageCopy[i] = OriginalImage[i];
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
		l_click = (l_click+1)%2;
		index = 0;
		if (l_click == 0 && points == 1)	//if points collection is completed
		{
			//resampling
			resampling();
			active_contour_rb();
			//contour_draw();

		}
		
		/*
		x_mouse = LOWORD(lParam);
		y_mouse = HIWORD(lParam);
		sprintf(text, "mouse click %d,%d     ", x_mouse, y_mouse);
		hDC = GetDC(MainWnd);
		TextOut(hDC, 0, 0, text, strlen(text));
		ReleaseDC(MainWnd, hDC);
		ThreadRegion = 1;
		RegionNumber++;
		//_beginthread(RegionGrowThread, 0, MainWnd);
		*/
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
		


	case WM_RBUTTONDOWN:
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);
		if (xPos >= 0 && xPos < COLS && yPos >= 0 && yPos < ROWS)
		{
			x_c = xPos; y_c = yPos;
			sprintf(text, "%d,%d=>%d     ", xPos, yPos, OriginalImage[yPos * COLS + xPos]);
			hDC = GetDC(MainWnd);
			TextOut(hDC, 0, 0, text, strlen(text));		/* draw text on the window */
			SetPixel(hDC, xPos, yPos, RGB(255, 0, 0));	/* color the cursor position red */
			ReleaseDC(MainWnd, hDC);
			ballon_point_calc();
			contour_draw();
		}
		active_contour_ballon();

		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	
	case WM_MOUSEMOVE:
		if (l_click == 1)
		{
			points = 1;	//indicating the collection of points has started
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
			if (xPos >= 0 && xPos < COLS && yPos >= 0 && yPos < ROWS)
			{
				x_pixel[index] = xPos;
				y_pixel[index] = yPos;
				sprintf(text, "%d,%d=>%d     ", xPos, yPos, OriginalImage[yPos * COLS + xPos]);
				hDC = GetDC(MainWnd);
				TextOut(hDC, 0, 0, text, strlen(text));		/* draw text on the window */
				SetPixel(hDC, xPos, yPos, RGB(255, 0, 0));	/* color the cursor position red */
				ReleaseDC(MainWnd, hDC);
			}
		}
		index++;
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

void resampling()
{
	int i = 0;
	while (x_pixel[i] != 0 && y_pixel[i] != 0)
	{
		px[i / SAMPLE_SIZE] = x_pixel[i];
		py[i / SAMPLE_SIZE] = y_pixel[i];
		i = i + SAMPLE_SIZE;
	}
	total_points = i / SAMPLE_SIZE;
}

void contour_draw()
{
	HDC		hDC;
	int c, r;	//to print plus on the contour points
	
	for (int i = 0; i < total_points; i++)
	{
		c = px[i];
		for (int dr = -3; dr < 4; dr++)
		{
			r = py[i] + dr;
			hDC = GetDC(MainWnd);
			SetPixel(hDC, c, r, RGB(255, 0, 0));	/* color the contour point position red */
			ReleaseDC(MainWnd, hDC);
			//OriginalImage[r * COLS + c] = 0;
		}

		r = py[i];
		for (int dc = -3; dc < 4; dc++)
		{
			c = px[i] + dc;
			hDC = GetDC(MainWnd);
			SetPixel(hDC, c, r, RGB(255, 0, 0));	/* color the contour point position red */
			ReleaseDC(MainWnd, hDC);
			//OriginalImage[r * COLS + c] = 0;
		}
		
	}
	//PaintImage();
}


void sobel_edge()
{
	int Gx[9], Gy[9];       //Sobel template
	float sum_x = 0.0, sum_y = 0.0;  //used for sobel edge detection
	float value = 0.0;      //used for sobel edge calculations

	//horizontal filter
	Gx[0] = 1; Gx[1] = 0; Gx[2] = -1;
	Gx[3] = 2; Gx[4] = 0; Gx[5] = -2;
	Gx[6] = 1; Gx[7] = 0; Gx[8] = -1;
	//vertical filter
	Gy[0] = 1; Gy[1] = 2; Gy[2] = 1;
	Gy[3] = 0; Gy[4] = 0; Gy[5] = 0;
	Gy[6] = -1; Gy[7] = -2; Gy[8] = -1;

	for (int r = 0; r < ROWS; r++)
		for (int c = 0; c < COLS; c++)
		{
			//printf("r is %d c is %d\n",r,c);
			sum_x = 0.0; sum_y = 0.0;
			if (r == 0 || r == (ROWS - 1) || c == 0 || c == (COLS - 1))
			{
				sum_x = 0.0; sum_y = 0; value = 0.0;
			}
			else
			{
				for (int rs = -1; rs <= 1; rs++)
				{
					for (int cs = -1; cs <= 1; cs++)
					{
						sum_x = sum_x + OriginalImage[(r + rs) * COLS + (c + cs)] * Gx[(1 + rs) * 3 + (1 + cs)];
						sum_y = sum_y + OriginalImage[(r + rs) * COLS + (c + cs)] * Gy[(1 + rs) * 3 + (1 + cs)];
					}
				}
				sum_x = sum_x / 9.0;
				sum_y = sum_y / 9.0;
				value = (double)sqrt(SQR(sum_x) + SQR(sum_y));
			}
			sobel_image[r * COLS + c] = (int)round(value);
		}

	//normalizing & saving sobel image
	normalizer_unsigned_char(sobel_image, ROWS * COLS, 255, 0);


	//for (int i = 0; i < ROWS * COLS; i++)
	//	OriginalImage[i] = sobel_image[i];

	for (int i = 0; i < ROWS * COLS; i++)
		edge_image[i] = -sobel_image[i] * sobel_image[i];

	//normalizing edge image to be used as input for external energy  
	normalizer_double(edge_image, ROWS * COLS, 1.0, 0.0);
	//PaintImage();
}


void active_contour_rb()
{
	double avg_dist;
	double internal_energy_1[ENERGY_WINDOW * ENERGY_WINDOW];    //internal energy due to square of distance
	double internal_energy_2[ENERGY_WINDOW * ENERGY_WINDOW];
	double external_energy[ENERGY_WINDOW * ENERGY_WINDOW];
	double total_energy[ENERGY_WINDOW * ENERGY_WINDOW];
	sobel_edge();
	for (int iteration = 1; iteration <= MAX_ITERATIONS; iteration++)
	{
		avg_dist = avg_dist_calc();
		//iterating through each point to find the least energy spot to move to
		for (int point = 0; point < total_points; point++)
		{
			
			energy_calc(point,avg_dist,internal_energy_1,internal_energy_2, external_energy);
			
			normalizer_double(internal_energy_1, ENERGY_WINDOW * ENERGY_WINDOW, 1.0, 0.0);//normalization of internal energy 1
			normalizer_double(internal_energy_2, ENERGY_WINDOW * ENERGY_WINDOW, 1.0, 0.0);//normalization of internal energy 2
			//total energy of the window
			for (int i = 0; i < ENERGY_WINDOW * ENERGY_WINDOW; i++)
				total_energy[i] = internal_energy_1[i] + internal_energy_2[i] +  external_energy[i];
			new_pos_calc(point, total_energy);

		}
		copy_image(ImageCopy, OriginalImage);
		PaintImage();
		contour_draw();
		Sleep(100);		/* pause 100 ms */
	}

}




void active_contour_ballon()
{
	double avg_dist;
	double internal_energy_1_b[ENERGY_WINDOW * ENERGY_WINDOW];    //internal energy due to centroid distance 
	double internal_energy_2_b[ENERGY_WINDOW * ENERGY_WINDOW];
	double external_energy_b[ENERGY_WINDOW * ENERGY_WINDOW];
	double total_energy_b[ENERGY_WINDOW * ENERGY_WINDOW];
	sobel_edge();
	for (int iteration = 1; iteration <= MAX_ITERATIONS; iteration++)
	{
		avg_dist = avg_dist_calc();
		centroid_calc();
		//iterating through each point to find the least energy spot to move to
		for (int point = 0; point < total_points; point++)
		{

			energy_calc_ballon(point, avg_dist, internal_energy_1_b, internal_energy_2_b, external_energy_b);

			normalizer_double(internal_energy_1_b, ENERGY_WINDOW * ENERGY_WINDOW, 1.0, 0.0);//normalization of internal energy 1
			normalizer_double(internal_energy_2_b, ENERGY_WINDOW * ENERGY_WINDOW, 1.0, 0.0);//normalization of internal energy 2
			//total energy of the window
			for (int i = 0; i < ENERGY_WINDOW * ENERGY_WINDOW; i++)
				total_energy_b[i] = 1*internal_energy_1_b[i] + 1*internal_energy_2_b[i] + 2*external_energy_b[i];
			new_pos_calc(point, total_energy_b);

		}
		copy_image(ImageCopy, OriginalImage);
		PaintImage();
		contour_draw();
		Sleep(100);		/* pause 100 ms */
	}

}


void energy_calc_ballon(int point, double avg_dist, double* IE_1, double* IE_2, double* EE)
{
	int next, prev;	//next & previous contour points
	int c, r;	//col & row of the contour point pixel
	double dist_centroid = 0.0; //distance between centroid and move to point within the window
	double dist_behind = 0.0, dist_ahead = 0.0;	//distance between current and previous waypoint , current and next waypoint
	for (int y = 0; y < ENERGY_WINDOW; y++)
	{
		for (int x = 0; x < ENERGY_WINDOW; x++)
		{
			next = (point + 1) % total_points;   //since it is a closed contour , the next point to last point is firsr point
			prev = (point - 1 + total_points) % total_points;
			r = (py[point] - ENERGY_WINDOW / 2) + y;
			c = (px[point] - ENERGY_WINDOW / 2) + x;
			if (r < 0 || r >= ROWS || c<0 || c>COLS)
				continue;
			dist_centroid = sqrt((double)SQR(r - c_y) + SQR(c - c_x));  //distance between centroid and move to point
			dist_ahead = sqrt((double)SQR(r - py[prev]) + SQR(c - px[prev]));
			dist_behind = sqrt((double)SQR(r - py[next]) + SQR(c - px[next]));
			*(IE_1 + y * ENERGY_WINDOW + x) = 0 - dist_centroid;// SQR(r - py[next]) + SQR(c - px[next]);
			*(IE_2 + y * ENERGY_WINDOW + x) = fabs(dist_ahead - dist_behind); // SQR(avg_dist - sqrt(*(IE_1 + y * ENERGY_WINDOW + x)));
			*(EE + y * ENERGY_WINDOW + x) = edge_image[r * COLS + c];
		}
	}
}

double avg_dist_calc()
{
	double avg_dist = 0;
	int next;	//next contour point
	for (int i = 0; i < total_points; i++)
	{
		next = (i + 1) % total_points;   //since it is a closed contour , the next point to last point is firsr point 
		float distance = sqrt(SQR(px[i] - px[next]) + SQR(py[i] - py[next]));
		avg_dist = avg_dist + (double)distance;
	}

	avg_dist = avg_dist / total_points;
	return avg_dist;
}

void centroid_calc()
{
	for (int i = 0; i < total_points; i++)
	{
		c_x = c_x + px[i];
		c_y = c_y + py[i];
	}
	c_x = c_x / total_points;
	c_y = c_y / total_points;
}

void energy_calc(int point, double avg_dist, double* IE_1, double* IE_2, double* EE)
{
	int next;	//next contour point
	int c, r;	//col & row of the contour point pixel
	for (int y = 0; y < ENERGY_WINDOW; y++)
	{
		for (int x = 0; x < ENERGY_WINDOW; x++)
		{
			next = (point + 1) % total_points;   //since it is a closed contour , the next point to last point is firsr point
			r = (py[point] - ENERGY_WINDOW / 2) + y;
			c = (px[point] - ENERGY_WINDOW / 2) + x;
			*(IE_1 + y * ENERGY_WINDOW + x) = SQR(r - py[next]) + SQR(c - px[next]);
			*(IE_2 + y * ENERGY_WINDOW + x) = SQR(avg_dist - sqrt(*(IE_1 + y * ENERGY_WINDOW + x)));
			*(EE + y * ENERGY_WINDOW + x) = edge_image[r * COLS + c];
		}
	}
}


void ballon_point_calc()
{
	total_points = 360 / ang_resol;
	int theta = 0;
	double dx, dy;
	for (int i = 0; i < total_points; i++)
	{
		dx = rad * cos(theta * M_PI / 180);
		dy = rad * sin(theta * M_PI / 180);
		px[i] = x_c + (int)dx;
		py[i] = y_c + (int)dy;
		theta = theta + ang_resol;
	}
}

void new_pos_calc(int point, double* TE)
{
	double max_f = 0.0;
	double data_f = 0.0;
	int destination_id = ENERGY_WINDOW * ENERGY_WINDOW / 2;	//index of lowest energy cell in the window
	double min_f = *(TE + ENERGY_WINDOW * ENERGY_WINDOW / 2);
	for (int i = 0; i < ENERGY_WINDOW * ENERGY_WINDOW; i++)
	{
		data_f = *(TE+i);
		if (data_f < min_f)
		{
			min_f = data_f;
			destination_id = i;
		}
	}
	int r = (destination_id / ENERGY_WINDOW) + 1;
	int c = (destination_id % ENERGY_WINDOW) + 1;
	py[point] = py[point] + r - ENERGY_WINDOW / 2 - 1;
	px[point] = px[point] + c - ENERGY_WINDOW / 2 - 1;
}

void normalizer_unsigned_char(unsigned char* array, int length, double n_max, double n_min) //for normalization
{
	int i;    //iteration purposes
	double max = -9999999.0, min = 9999999.0;    //min & max initialization
	double data;  //variable to store array values for comparison

	for (int i = 0; i < length; i++) //to find the min and max values
	{
		data = *(array + i);
		if (data > max)
			max = data;
		if (data < min)
			min = data;
	}
	//limiting the min and max to n_min and n_max values
	for (i = 0; i < length; i++)
		*(array + i) = (unsigned char)(((*(array + i) - min) * (n_max - n_min) / (max - min)) + n_min);
}


void normalizer_double(double* array, int length, double n_max, double n_min) //for normalization
{
	int i;    //iteration purposes
	double max = -9999999.0, min = 9999999.0;    //min & max initialization
	double data;  //variable to store array values for comparison

	for (int i = 0; i < length; i++) //to find the min and max values
	{
		data = *(array + i);
		if (data > max)
			max = data;
		if (data < min)
			min = data;
	}
	//printf("max is %lf min is %lf\n",max, min);
	//limiting the min and max to n_min and n_max values
	for (i = 0; i < length; i++)
		*(array + i) = (double)(((*(array + i) - min) * (n_max - n_min) / (max - min)) + n_min);
}


void copy_image(unsigned char* source, unsigned char* destination)
{
	for (int i = 0; i < ROWS * COLS; i++)
		*(destination + i) = *(source + i);
}

void print_rc(double* array, int ROWS, int COLS)
{
	for (int r = 0; r < ROWS; r++)
	{
		for (int c = 0; c < COLS; c++)
			printf("%f ", *(array + (r * COLS) + c));
		printf("\n");
	}
}


void RegionGrowStep(int id)
{
	HDC hDC;
	hDC = GetDC(MainWnd);
	int r = indices[id] / COLS;
	int c = indices[id] % COLS;
	SetPixel(hDC, c, r, SegColor);	/* color the cursor position red */
	ReleaseDC(MainWnd, hDC);
	OriginalImage[indices[index]] = SegColor;
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
	sprintf(text, "average %lf variance %lf     ", avg, var);
	hDC = GetDC(MainWnd);
	TextOut(hDC, 50, 50, text, strlen(text));		/* draw text on the window */
	ReleaseDC(MainWnd, hDC);



	//checking whether the current pixel can have any potential region around it
	if (var > 1.0)
		return;



	sprintf(text, "zone started  %d  %d  ", region_id, label[r * COLS + c]);
	hDC = GetDC(MainWnd);
	TextOut(hDC, 70, 70, text, strlen(text));		/* draw text on the window */
	ReleaseDC(MainWnd, hDC);

	
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

