
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

int main(int argc, char *argv[])
{
FILE		*fpt;
unsigned char	*image;
float *intermediate;
unsigned char	*smoothed;
char		header[320];
int		ROWS,COLS,BYTES;
int		r,c,r2,c2;
float sum;
struct timespec	tp1,tp2;
int data;

if (argc != 2)
  {
  printf("Usage:  lab1_1 [filename] \n");
  exit(0);
  }

	/* read image */
if ((fpt=fopen(argv[1],"rb")) == NULL)
  {
  // std::cout<<"Unable to open "<<argv[1]<<" for reading\n";  
  printf("Unable to open %s for reading\n",argv[1]);
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
intermediate=(float *)calloc(ROWS*COLS,sizeof(float));

	/* query timer */
clock_gettime(CLOCK_REALTIME,&tp1);
printf("Seperable filter(1x7 & 7x1)\n");
// printf("%ld %ld\n",(long int)tp1.tv_sec,tp1.tv_nsec);

	/* smooth image, skipping the border points */

for (r=0; r<ROWS; r++)
{  
  for (c = 0; c<COLS; c++)
  {
    sum=0;
    if (c<3 || c>=COLS-3)
      sum=0;
    else
      for (c2=-3; c2<=3; c2++ )
        sum+=(image[(r)*COLS+(c+c2)]);
    intermediate[r*COLS+c]=sum/7.0;
  }
}

for (c=3; c<COLS-3; c++)
{
  for (r = 3; r<ROWS-3; r++)
  {
    sum=0;
    if (r<3 || r>=ROWS-3)
      sum=0;
    else
      for (r2=-3; r2<=3; r2++ )
        sum+=(intermediate[(r+r2)*COLS+(c)]);
    data=round(sum/7.0);
    smoothed[r*COLS+c] = data;
  }
}


	/* query timer */
clock_gettime(CLOCK_REALTIME,&tp2);
// printf("%ld %ld\n",(long int)tp2.tv_sec,tp2.tv_nsec);

	/* report how long it took to smooth */
printf("Processing time %ld milliseconds\n",(tp2.tv_nsec-tp1.tv_nsec)/1000000);
printf("Output: smoothed_seperable.ppm\n");

	/* write out smoothed image to see result */
fpt=fopen("smoothed_seperable.ppm","wb");
fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
fwrite(smoothed,COLS*ROWS,1,fpt);
fclose(fpt);
return 0;
}