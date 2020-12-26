#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main (int argc, char *argv[])
{
    FILE *fpt;
    unsigned char *image, *detection, *msf_normalized, *detected_image; 
    char		header[320];
    int		ROWS,COLS,BYTES;  //parameters of the image
    int threshold;
    char file_name_bin[30], file_name_img[30]; //output file name
    int tr, tc; //row and col of the image which are above threshold value

    if (argc != 4)
    {
        printf("Usage exe_name [original_image] [msf_normalized] [threshold]");
        exit(0);
    }

    threshold = atoi(argv[3]);
    printf("threshold is %d\n",threshold);
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
    detected_image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
    header[0]=fgetc(fpt);	/* read white-space character that separates header */
    fread(image,1,COLS*ROWS,fpt);
    fclose(fpt);
    detected_image = image;
    // for (int i = 0; i<ROWS*COLS; i++)
    //     *detected_image[i] = *image[i];
    if ((fpt=fopen(argv[2],"rb")) == NULL)
    {
    // std::cout<<"Unable to open "<<argv[1]<<" for reading\n";  
    printf("Unable to open %s for reading\n",argv[2]);
    exit(0);
    }
    fscanf(fpt,"%s %d %d %d",header,&COLS,&ROWS,&BYTES);
    if (strcmp(header,"P5") != 0  ||  BYTES != 255)
    {
    printf("Not a greyscale 8-bit PPM image\n");
    exit(0);
    }
    msf_normalized=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
    header[0]=fgetc(fpt);	/* read white-space character that separates header */
    fread(msf_normalized,1,COLS*ROWS,fpt);
    fclose(fpt);



    detection = (unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
    for (int i=0; i<ROWS*COLS; i++)
    {
        if (msf_normalized[i] >= threshold)
        {
            detection[i] = 255;
            tr = i/COLS;
            tc = i%COLS;
            for (int r=-7; r<=7; r++)
            {
                detected_image[(r+tr)*COLS + tc-4] = 255; //left line
                detected_image[(r+tr)*COLS + tc+4] = 255; //right line
            }
                
            for(int c=-4; c<=4; c++)
            {
                detected_image[(tr-7)*COLS + c+tc] = 255;    //top line
                detected_image[(tr+7)*COLS + c+tc] = 255;    //bottom line
            }
        }
            
        else
            detection[i] = 0;
    }

    strcpy(file_name_bin,"bin_img_threshold_");
    char threshold_char[5];
    itoa(threshold,threshold_char,10);
    strcat(file_name_bin,threshold_char);
    strcat(file_name_bin,".ppm");
    // + tostring(threshold) + ".ppm"; 
    fpt = fopen(file_name_bin,"wb");
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(detection,COLS*ROWS,1,fpt);
    fclose(fpt);

    strcpy(file_name_img,"det_img_threshold_");
    strcat(file_name_img,threshold_char);
    strcat(file_name_img,".ppm");
    fpt = fopen(file_name_img,"wb");
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(detected_image,COLS*ROWS,1,fpt);
    fclose(fpt);

    printf("image loaded\n");
}