
#define SQR(x) ((x)*(x))	/* macro for square */
#ifndef M_PI			/* in case M_PI not found in math.h */
#define M_PI 3.1415927
#endif
#ifndef M_E
#define M_E 2.718282
#endif

#define MAX_FILENAME_CHARS	320
#define MAX_PIXEL 100000
#define SAMPLE_SIZE 20
#define MAX_ITERATIONS 50
#define ENERGY_WINDOW 15			//rubber band - 7

char	filename[MAX_FILENAME_CHARS];

HWND	MainWnd;

		// Display flags
int		ShowPixelCoords;
int		PlayMode, StepMode;

		// Image data
unsigned char	*OriginalImage, *label, *ImageCopy, *OriginalImageRGB;
unsigned char *sobel_image, * temp;
double* edge_image;   //variable to store the normalized negated edge detected image
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

//active contour
int l_click = 0;	//flag indicating whether contour points must be drawn with left click
int x_pixel[MAX_PIXEL], y_pixel[MAX_PIXEL]; //recording pixel values traversed by cursor
int px[MAX_PIXEL / SAMPLE_SIZE], py[MAX_PIXEL / SAMPLE_SIZE];	//resampled set of contour points
int index;	//index of array of pixels
int points;	//flag indicating the collection of contour points
int total_points;	//total number of contour points
int sobel = 0;
int rad = 10; //radius of circle for ballon model
int ang_resol = 20;	//angular resolution between two contour points in ballon model
int x_c, y_c;	//center of circle for ballon model
int c_x, c_y;	//centroid of set of contour points

		// Function prototypes
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
void PaintImage();
void AnimationThread(void *);		/* passes address of window */
void active_contour_rb();	//rubber band model
void active_contour_ballon();	//ballon model

void resampling();	//to resample the left click contour points
void contour_draw();	//to draw the contour points
void sobel_edge();	//to perform Sobel edge detectio on the image
void energy_calc(int point, double avg_dist, double* IE_1, double* IE_2, double* EE);	//to calculate internaal energies 1 & 2, external energy
void new_pos_calc(int point, double* TE);	//to determine the new position of current contour point based on least energy

void ballon_point_calc();
void energy_calc_ballon();	//energy calculation for ballon model

double avg_dist_calc();	//to calculate average distance between contour points
void centroid_calc();
void normalizer_double(double* array, int length, double n_max, double n_min); //for normalization
void normalizer_unsigned_char(unsigned char* array, int length, double n_max, double n_min); //for normalization
void copy_image(unsigned char * source, unsigned char *destination);	//to reset the contents of originalimage


void RegionGrowThread(void*);		//to grow region around a clicked pixel
void RegionGrowPlay(void*);			//to fill the segment in continious manner
void RegionGrowStep(int id);		//to fill the segment in step manner
void SettingsDialogBox();					//dialog box for user to enter predicate values