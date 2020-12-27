// you forgot to negate the pixels for sobel filter do it first before you do anything else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SQR(x) ((x)*(x))


void normalizer_unsigned_char(unsigned char *array, int length, double n_max, double n_min) //for normalization
{
  int i;    //iteration purposes
  double max = -9999999.0, min = 9999999.0;    //min & max initialization
  double data;  //variable to store array values for comparison
  
  for(int i=0; i<length; i++) //to find the min and max values
  {
    data = *(array + i);
    if (data > max)
      max = data;
    if (data < min)
      min = data;
  }
  //limiting the min and max to n_min and n_max values
  for(i = 0; i<length; i++)
      *(array+i) = (unsigned char)(((*(array+i) - min) * (n_max - n_min) / (max - min)) + n_min);
}


void normalizer_double(double *array, int length, double n_max, double n_min) //for normalization
{
  int i;    //iteration purposes
  double max = -9999999.0, min = 9999999.0;    //min & max initialization
  double data;  //variable to store array values for comparison
  
  for(int i=0; i<length; i++) //to find the min and max values
  {
    data = *(array + i);
    if (data > max)
      max = data;
    if (data < min)
      min = data;
  }
  //printf("max is %lf min is %lf\n",max, min);
  //limiting the min and max to n_min and n_max values
  for(i = 0; i<length; i++)
      *(array+i) = (double)(((*(array+i) - min) * (n_max - n_min) / (max - min)) + n_min);
}

void print_rc(double *array, int ROWS, int COLS)
{
  for (int r = 0; r< ROWS; r++)
  {
    for (int c = 0; c<COLS; c++)
      printf("%f ",*(array+ (r*COLS) + c));
    printf("\n");
  }
}

int main(int argc, char *argv[])

{
FILE	*fpt;
int	px[100],py[100];
int px_temp[100], py_temp[100];   //temporary position during each iteration
int	i,total_points;
int	window = 7,x,y,move_x,move_y;
double c_x = 0.0, c_y = 0.0;     //coordinates of centroid of contour points
int r,c;    //cooridnates of points moving to
double dist_centroid = 0.0; //distance between centroid and move to point within the window
unsigned char *image, *temp, *sobel_image;
double *edge_image;   //variable to store the normalized negated edge detected image
char header[320];
int ROWS, COLS, BYTES;
double avg_dist = 0.0;    //average distance between contour points
double internal_energy_1[7*7];    //internal energy due to square of distance
double internal_energy_2[7*7];
double external_energy[7*7];
double total_energy[7*7];
int next;       //next contour point index
int min, max;   //for normalization purposes integers
double min_f, max_f;  //normalization purposes double
int data;       //for normalization purposes int
double data_f;     //for normalization purposes double
int n_max , n_min ;   //limiting the max and min values of energy
double n_max_f, n_min_f;   //new max and min in double
int Gx[9], Gy[9];       //Sobel template
float sum_x =0.0, sum_y = 0.0;  //used for sobel edge detection
float value = 0.0;      //used for sobel edge calculations
int max_iteration = 30;   //maximum number of iterations we can perform
int destination_id;   //the cell to which the contour point will move towards in current iteration

if (argc != 1)
  {
  printf("Usage:  lab5 \n");
  exit(0);
  }



	/* read contour points file */
if ((fpt=fopen("initial_points.txt","r")) == NULL)
  {
  printf("Unable to find initial_points.txt for reading\n",argv[1]);
  exit(0);
  }
total_points=0;
while (1)
  {
  i=fscanf(fpt,"%d %d",&px[total_points],&py[total_points]);
  if (i != 2)
    break;
  total_points++;
  if (total_points > 100)
    break;
  }
fclose(fpt);

fpt = fopen("hawk.ppm","rb");
if (fpt == NULL)
    printf("hawk.ppm missing");

i = fscanf(fpt, "%s %d %d %d ", header, &COLS, &ROWS, &BYTES);
image = (unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char ));
temp = (unsigned char *)calloc((ROWS+2)*(COLS+2), sizeof(unsigned char ));
sobel_image = (unsigned char *)calloc(ROWS*COLS, sizeof(unsigned char ));
edge_image = (double *)calloc(ROWS*COLS, sizeof(double));
fread(image, 1, ROWS*COLS, fpt);
fclose(fpt);

for(i = 0; i<ROWS*COLS; i++)
  temp[i] = image[i];

//marking active contour points on image
for(i = 0; i<total_points; i++)
{
  c = px[i];
  for(int dr=-3; dr<4; dr++)
  {
    r = py[i] + dr;
    temp[r*COLS + c] = 0; 
  }

  r=py[i];
  for(int dc = -3; dc<4; dc++)
  {
    c=px[i] + dc;
    temp[r*COLS + c] = 0;
  }
}

fpt = fopen("active_contour_initial.ppm","wb");
fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
fwrite(temp, (ROWS)*(COLS), 1, fpt);
fclose(fpt);

//resetting temp to original image
for(i = 0; i<ROWS*COLS; i++)
  temp[i] = image[i];

//sobel edge detection
//horizontal filter
Gx[0] = 1;Gx[1] = 0;Gx[2] = -1;
Gx[3] = 2;Gx[4] = 0;Gx[5] = -2;
Gx[6] = 1;Gx[7] = 0;Gx[8] = -1; 
//vertical filter
Gy[0] = 1;Gy[1] = 2;Gy[2] = 1;
Gy[3] = 0;Gy[4] = 0;Gy[5] = 0;
Gy[6] = -1;Gy[7] = -2;Gy[8] = -1; 


for(int r = 0; r<ROWS; r++)
  for(int c = 0; c<COLS; c++)
  {
    //printf("r is %d c is %d\n",r,c);
    sum_x = 0.0; sum_y = 0.0;
    if (r == 0 || r==(ROWS-1) || c == 0 || c==(COLS-1) )
    {
      sum_x = 0.0; sum_y = 0; value = 0.0;
    }  
    else
    {
      for(int rs = -1; rs<=1; rs++)
      {
        for(int cs = -1; cs<=1; cs++)
        {
          sum_x = sum_x + image[(r+rs)*COLS + (c+cs)] * Gx[(1+rs)*3 + (1+cs)];
          sum_y = sum_y + image[(r+rs)*COLS + (c+cs)] * Gy[(1+rs)*3 + (1+cs)];
        }
      }
      sum_x = sum_x/9.0;
      sum_y = sum_y/9.0;
      value = (double)sqrt(SQR(sum_x) + SQR(sum_y));
    }
    sobel_image[r*COLS + c] = (int)round(value);
  }


//normalizing & saving sobel image
normalizer_unsigned_char(sobel_image,ROWS*COLS, 255, 0);
fpt = fopen("sobel.ppm","wb");
fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
fwrite(sobel_image, (ROWS)*(COLS), 1, fpt);
fclose(fpt);

/*
fpt = fopen("edge.txt","w");
float data_s;
for (r = 0; r<ROWS; r++)
{
  for (c = 0; c<COLS; c++)
    fprintf(fpt,"%d ",sobel_image[r*COLS + c]);
  fprintf(fpt,"\n"); 
}
fclose(fpt);
*/

for(i = 0; i<ROWS*COLS; i++)
  edge_image[i] = - sobel_image[i] * sobel_image[i];
  
//normalizing edge image  
normalizer_double(edge_image,ROWS*COLS, 1.0, 0.0);


// iterations begin   
for (int iteration = 1; iteration <= max_iteration; iteration++)
{
  //calculation of average distance between points
  
  avg_dist = 0;
  for (i = 0; i<total_points; i++)
  {
    next = (i+1) % total_points;   //since it is a closed contour , the next point to last point is firsr point 
    float distance =sqrt(SQR(px[i] - px[next]) + SQR(py[i] - py[next]) ); 
    avg_dist = avg_dist + (double)distance ;
  }
  
  avg_dist = avg_dist / total_points;

  //iterating through each point to find the least energy spot to move to
  for(int point = 0; point<total_points; point++)
  {
    //printf("start\n");
    for (y=0; y<window; y++)
    {
      for (x=0; x<window; x++)
        {
          
          next = (point+1) % total_points;   //since it is a closed contour , the next point to last point is firsr point
          r = (py[point] - window/2) + y;
          c = (px[point] - window/2) + x;
          internal_energy_1[y*window+x] = SQR(r-py[next]) + SQR(c-px[next]);
          internal_energy_2[y*window+x] = SQR(avg_dist - sqrt(internal_energy_1[y*window+x])); 
          external_energy[y*window+x] = edge_image[r*COLS + c];  
          
        }
    }
    
  
    
    //normalization of internal energy 1
    normalizer_double(internal_energy_1,window*window, 1.0, 0.0);
    //normalization of internal energy 2
    normalizer_double(internal_energy_2,window*window, 1.0, 0.0);

    
    //total energy of the window
    for (i = 0; i<window*window; i++)
        total_energy[i] = internal_energy_1[i] + internal_energy_2[i] + 1*external_energy[i];

    
    max_f = 0.0; min_f = 99999999.0;
    
    for (i = 0; i<window*window; i++)
    {
      data_f = total_energy[i];
      if (data_f < min_f)
      {
        min_f = data_f;
        destination_id = i;
      }
    }
    r = (destination_id/window) + 1;
    c = (destination_id % window) + 1;
    py[point] = py[point] + r - window/2 - 1;
    px[point] = px[point] + c - window/2 - 1;

  }

  
}




//marking active contour points on image
for(i = 0; i<total_points; i++)
{
  c = px[i];
  for(int dr=-3; dr<4; dr++)
  {
    r = py[i] + dr;
    temp[r*COLS + c] = 0; 
  }

  r=py[i];
  for(int dc = -3; dc<4; dc++)
  {
    c=px[i] + dc;
    temp[r*COLS + c] = 0;
  }
}

fpt = fopen("output.ppm","wb");
fprintf(fpt, "P5 %d %d 255\n", COLS, ROWS);
fwrite(temp, (ROWS)*(COLS), 1, fpt);
fclose(fpt);

}


/*
printf("current x %d current y %d\n",px[i],py[i]);
printf("next x is %d next y is %d\n",px[next], py[next]);
*/


/*
fpt = fopen("edge.txt","w");
float max_s = 0.0, min_s = 99999999.0;
float data_s;
for (i = 0; i<ROWS*COLS; i++)
{
    data_s = edge_image[i];
    char data_c = (char)data_s;
    //if (data_s > 2)
    fprintf(fpt,"%f\n",data_s);
}
fclose(fpt);
printf("min is %lf max is %lf",min_s, max_s);
*/


/*
for (r = 0; r<ROWS; r++)
  {
  for(c = 0; c<COLS; c++)
    printf("%f ",edge_image[r*COLS + c]);
  printf("\n");
  }
*/
