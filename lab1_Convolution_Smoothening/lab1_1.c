
	/*
	** This program reads bridge.ppm, a 512 x 512 PPM image.
	** It smooths it using a standard 3x3 mean filter.
	** The program also demonstrates how to time a piece of code.
	**
	** To compile, must link using -lrt  (man clock_gettime() function).
	*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

int main()

{
FILE		*fpt;
unsigned char	*image;
unsigned char	*smoothed;
char		header[320];
int		ROWS,COLS,BYTES;
int		r,c,r2,c2;
float sum;
struct timespec	tp1,tp2;
int data;

	/* read image */
if ((fpt=fopen("bridge.ppm","rb")) == NULL)
  {
  printf("Unable to open bridge.ppm for reading\n");
  exit(0);
  }
fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
if (strcmp(header,"P5") != 0  ||  BYTES != 255)
  {
  printf("Not a greyscale 8-bit PPM image\n");
  exit(0);
  }
image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
header[0]=fgetc(fpt);	/* read white-space character that separates header */
fread(image,1,COLS*ROWS,fpt);
fclose(fpt);

	/* allocate memory for smoothed version of image */
smoothed=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));

	/* query timer */
clock_gettime(CLOCK_REALTIME,&tp1);
printf("2d Convolution 7x7 mean filter\n");
//printf("Start time in %ld s %ld ns\n",(long int)tp1.tv_sec,tp1.tv_nsec);

	/* smooth image, skipping the border points */
for (r=0; r<ROWS; r++)
  for (c=0; c<COLS; c++)
    {
    sum=0;
    if (r<3 || r >= ROWS -3 || c<3 || c>=COLS-3)
      sum = 0;
    else
      for (r2=-3; r2<=3; r2++)
        for (c2=-3; c2<=3; c2++)
          sum=sum + (image[(r+r2)*COLS+(c+c2)]);
    data = round (sum/49.0);
    smoothed[r*COLS+c]=data;
    }

	/* query timer */
clock_gettime(CLOCK_REALTIME,&tp2);
//printf("Finish time %ld s %ld ns\n",(long int)tp2.tv_sec,tp2.tv_nsec);

	/* report how long it took to smooth */
printf("Processing time %ld milliseconds\n",(tp2.tv_nsec-tp1.tv_nsec)/1000000);
printf("Output: smoothed_2d_convolution.ppm\n");
	/* write out smoothed image to see result */
fpt=fopen("smoothed_2d_convolution.ppm","wb");
fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
fwrite(smoothed,COLS*ROWS,1,fpt);
fclose(fpt);
return 0;
}