
#define SQR(x) ((x)*(x))	/* macro for square */
#ifndef M_PI			/* in case M_PI not found in math.h */
#define M_PI 3.1415927
#endif
#ifndef M_E
#define M_E 2.718282
#endif

#define MAX_FILENAME_CHARS	320

char	filename[MAX_FILENAME_CHARS];

HWND	MainWnd;

		// Display flags
int		ShowPixelCoords;
int		PlayMode, StepMode;

		// Image data
unsigned char	*OriginalImage, *label, *ImageCopy, *OriginalImageRGB;
int				ROWS,COLS;
int				x_mouse, y_mouse;
int				RegionNumber;	//id to each each region
int* indices;	//to store index of pixels in a region
int index;	//used for filling in step grow mode
int count = 0;	//number of pixels in the region
int PlayMode = 0, StepMode = 0;
int SegDone = 0;	//flag to indicate whether segmentation process is over or not
int SegColor = 0;	//color of segmentation zone
int AvgDiff = 50;
double CentroidDist = 0.1;

#define TIMER_SECOND	1			/* ID of timer used for animation */

		// Drawing flags
int		TimerRow,TimerCol;
int		ThreadRow,ThreadCol;
int		ThreadRunning;
int		ThreadRegion;

		// Function prototypes
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
void PaintImage();
void AnimationThread(void *);		/* passes address of window */
void RegionGrowThread(void*);		//to grow region around a clicked pixel
void RegionGrowPlay(void*);			//to fill the segment in continious manner
void RegionGrowStep(int id);		//to fill the segment in step manner
void SettingsDialogBox();					//dialog box for user to enter predicate values