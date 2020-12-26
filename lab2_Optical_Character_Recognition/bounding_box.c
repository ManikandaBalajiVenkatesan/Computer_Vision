#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main (int argc, char *argv[])
{
    FILE *fpt;
    unsigned char *image;
    char		header[320];
    int		ROWS,COLS,BYTES;
    int tr,tc;      //tr-target row, tc-target coloumn about which we want to put box
    char letter;    //letter in our ground truth
    char d_letter;  //desired letter 

    if (argc != 2)
    {
        printf("Usage bounding_box [letter]");
        exit(0);
    }
            
    d_letter = argv[1][0];
    
    fpt = fopen("parenthood.ppm","rb");
    if (fpt == NULL)
    {
        printf("check for parenthood.ppm\n");
    }
    int i = fscanf(fpt,"%s %d %d %d ",header,&COLS,&ROWS,&BYTES);
    
    //header[0]=fgetc(fpt);
    image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
    
    fread(image,1,COLS*ROWS,fpt);
    fclose(fpt);
    
    fpt = fopen("ground_truth.txt","r");
    if (fpt == NULL)
    {
        printf("check for ground_truth.txt\n");
    }  
   
    while(1)
    {
        i = fscanf(fpt, "%c %d %d ",&letter, &tc, &tr);
        if (i != 3)
            break;
        if (letter == d_letter)
        {
            
            for (int r=-7; r<=7; r++)
            {
                image[(r+tr)*COLS + tc-4] = 255; //left line
                image[(r+tr)*COLS + tc+4] = 255; //right line
            }
                
            for(int c=-4; c<=4; c++)
            {
                image[(tr-7)*COLS + c+tc] = 255;    //top line
                image[(tr+7)*COLS + c+tc] = 255;    //bottom line
            }
            printf("%d\t%d\n",tr,tc);      
        }
        

    }
    fclose(fpt);

    fpt = fopen("outline_image.ppm","wb");
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(image,COLS*ROWS,1,fpt);
    fclose(fpt);
         
    
    

}