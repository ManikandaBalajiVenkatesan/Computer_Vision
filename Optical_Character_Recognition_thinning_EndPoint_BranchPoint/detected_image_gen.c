#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main (int argc, char *argv[])
{
    FILE *fpt;
    unsigned char *image, *detection, *msf_normalized, *detected_image, *template, *letter_image, *thinned_image; 
    char		header[320];
    int		ROWS,COLS,BYTES;  //parameters of the image
    int T;
    char file_name_bin[30], file_name_img[30]; //output file name
    int tr, tc; //row and col of the image which are above threshold value
    int l_ROWS, l_COLS;             //size of the letter image with padding
    int     t_ROWS,t_COLS,t_BYTES;  //parameters of the template
    int deletion;   //number of pixels to be erased for thinning
    unsigned char *erase_selection;
    int transition; //to record transition from edge to non edge for thinning
    int neighbors;  //to record number of edge neighbors around every edge pixle
    int NEWS;       //to record the north or east or (west and south) condition
    int end_points=0, branch_points=0;   //number of edge and branch points 
    char d_letter = 'e';            //letter to be detected

    if (argc != 2)
    {
        printf("Usage exe_name [threshold]");
        exit(0);
    }

    T = atoi(argv[1]);
    printf("threshold is %d\n",T);
    if ((fpt=fopen("parenthood.ppm","rb")) == NULL)
    {
    // std::cout<<"Unable to open "<<argv[1]<<" for reading\n";  
    printf("Unable to open parenthood.ppm for reading\n");
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
    thinned_image=(unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));
    header[0]=fgetc(fpt);	/* read white-space character that separates header */
    fread(image,1,COLS*ROWS,fpt);
    fclose(fpt);
    detected_image = image;

    if ((fopen("msf_e.ppm","rb")) == NULL)
    {
        printf("Unable to open msf_e.ppm for reading\n");
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
    
    //to open the template image
    fpt = fopen("parenthood_e_template.ppm","rb");
    if (fpt == NULL)
        printf("check for parenthood_e_template.ppm\n");

    int i = fscanf(fpt,"%s %d %d %d ",header,&t_COLS,&t_ROWS,&t_BYTES);
    template = (unsigned char *)calloc(t_ROWS*t_COLS,sizeof(unsigned char));
    fread(template,1,t_COLS*t_ROWS,fpt);
    fclose(fpt);
    l_ROWS = t_ROWS+2; l_COLS= t_COLS+2;//since we are have to check edge to non edge transition around every pixel we increase the size by 2

    detection = (unsigned char *)calloc(ROWS*COLS,sizeof(unsigned char));


    for (int i=0; i<ROWS*COLS; i++)
    {
        if (msf_normalized[i] >= T)
            detection[i] = 255;
        else
            detection[i] = 0;
    }
    
    for(int idx = 0; idx<ROWS*COLS; idx++)
    {        

        
        tr = idx/COLS; tc = idx%COLS;
        if (detection[idx] == 255)
        {
            
            letter_image = (unsigned char *)calloc(l_ROWS*l_COLS,sizeof(unsigned char));
            for (int r=-l_ROWS/2+1; r<=l_ROWS/2-1; r++)
                for(int c=-l_COLS/2+1; c<=l_COLS/2-1; c++)
                    letter_image[(r+l_ROWS/2)*l_COLS + (c+l_COLS/2)] = image[(tr+r)*COLS + (tc+c)];  

            //creating binary image, so that we can have edges
            for (i=0; i<l_ROWS*l_COLS; i++)
                if (letter_image[i]>128)
                    letter_image[i] = 0;
                else
                    letter_image[i] = 1;
            for(int r = 0; r<=l_ROWS; r = r+l_ROWS-1)
                for(int c=0; c<l_COLS; c++)
                    letter_image[r*l_COLS + c] = 0;
            for(int c = 0; c<=l_COLS; c = c+l_COLS-1)
                for(int r=0; r<l_ROWS; r++)
                    letter_image[r*l_COLS + c] = 0;
            
            int iteration = 0;
            //thinning process on the binary letter image
            deletion = 0;//count of pixels to be deleted
            while(1)
            {

                deletion=0;     
                erase_selection = (unsigned char *)calloc(l_ROWS*l_COLS,sizeof(unsigned char));
                for(int r=1; r<l_ROWS-1; r++)
                    for(int c=1; c<l_COLS-1; c++)
                        if (letter_image[r*l_COLS + c] == 1)
                        {
                            transition = 0; neighbors=0; NEWS=0;
                            //neighbour pixels
                            int n1 = letter_image[(r-1)*l_COLS + (c-1)];
                            int n2 = letter_image[(r-1)*l_COLS + c];
                            int n3 = letter_image[(r-1)*l_COLS + (c+1)];
                            int n4 = letter_image[r*l_COLS + (c+1)];
                            int n5 = letter_image[(r+1)*l_COLS + (c+1)];
                            int n6 = letter_image[(r+1)*l_COLS + c];
                            int n7 = letter_image[(r+1)*l_COLS + (c-1)];
                            int n8 = letter_image[r*l_COLS + (c-1)];
                            
                            //checking edge to non edge transition for every edge pixel except border pixels
                            if(n1-n2 == 1){transition++;}
                            if(n2-n3 == 1){transition++;}
                            if(n3-n4 == 1){transition++;}
                            if(n4-n5 == 1){transition++;}
                            if(n5-n6 == 1){transition++;}
                            if(n6-n7 == 1){transition++;}
                            if(n7-n8 == 1){transition++;}
                            if(n8-n1 == 1){transition++;}

                            //printf("r %d c %d transition  %d\n",r,c,transition);



                            //checking whether neighbour pixels are edge pixels
                            if(n1 == 1){neighbors++;}
                            if(n2 == 1){neighbors++;}
                            if(n3 == 1){neighbors++;}
                            if(n4 == 1){neighbors++;}
                            if(n5 == 1){neighbors++;}
                            if(n6 == 1){neighbors++;}
                            if(n7 == 1){neighbors++;}
                            if(n8 == 1){neighbors++;}
                            //printf("r %d c %d neighbors  %d\n",r,c,neighbors);

                            int north = n2;
                            int east = n4;
                            int south = n6;
                            int west = n8;

                            if (north != 1 || east != 1 || (west != 1 && south != 1) )
                                NEWS = 1;
                            
                            if (transition == 1 && neighbors >= 2 && neighbors <= 6 && NEWS == 1)
                            {
                                deletion++;
                                erase_selection[r*l_COLS + c] = 1;
                                //printf("r %d c %d transition %d neighbors %d NEWS %d\n",r,c,transition, neighbors, NEWS);
                            }
                        }
               
                if (deletion == 0)
                    break;

                for(i =0; i<l_ROWS*l_COLS; i++)
                    if (erase_selection[i] == 1)
                        letter_image[i] = 0;
                
                iteration++;
                deletion = 0;

            }
            
          
            for (int r=-l_ROWS/2+1; r<=l_ROWS/2-1; r++)
                for(int c=-l_COLS/2+1; c<=l_COLS/2-1; c++)
                    if(letter_image[(r+l_ROWS/2)*l_COLS + (c+l_COLS/2)] == 1)
                        thinned_image[(tr+r)*COLS + (tc+c)] = 255  ;
            //feature detection - end and branch points
            end_points = 0; branch_points =0;
            for(int r=1; r<l_ROWS-1; r++)
                for(int c=1; c<l_COLS-1; c++)
                    if (letter_image[r*l_COLS + c] == 1)
                    {
                        transition = 0;
                        
                        //neighbour pixels
                        int n1 = letter_image[(r-1)*l_COLS + (c-1)];
                        int n2 = letter_image[(r-1)*l_COLS + c];
                        int n3 = letter_image[(r-1)*l_COLS + (c+1)];
                        int n4 = letter_image[r*l_COLS + (c+1)];
                        int n5 = letter_image[(r+1)*l_COLS + (c+1)];
                        int n6 = letter_image[(r+1)*l_COLS + c];
                        int n7 = letter_image[(r+1)*l_COLS + (c-1)];
                        int n8 = letter_image[r*l_COLS + (c-1)];
                        
                        //checking edge to non edge transition for every edge pixel except border pixels
                        if(n1-n2 == 1){transition++;}
                        if(n2-n3 == 1){transition++;}
                        if(n3-n4 == 1){transition++;}
                        if(n4-n5 == 1){transition++;}
                        if(n5-n6 == 1){transition++;}
                        if(n6-n7 == 1){transition++;}
                        if(n7-n8 == 1){transition++;}
                        if(n8-n1 == 1){transition++;}

                        
                        if (transition == 1){end_points++;}//printf("end r %d c %d\n",r,c);}
                        if (transition > 2){branch_points++;}//printf("branch r %d c %d\n",r,c);}

                    }
            
            if (end_points != 1 && branch_points != 1 )
                detection[idx] = 0;
        }
    }
    //drawing bounding box on the original image around the detected letter
    for (int i=0; i<ROWS*COLS; i++)
    {
        if (detection[i] >= 255)
        {
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
    }

    char threshold_char[5];
    itoa(T,threshold_char,10);
    strcpy(file_name_img,"det_img_threshold_");
    strcat(file_name_img,threshold_char);
    strcat(file_name_img,".ppm");
    fpt = fopen(file_name_img,"wb");
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(detected_image,COLS*ROWS,1,fpt);
    fclose(fpt);

    strcpy(file_name_img,"thinned_img_threshold_");
    strcat(file_name_img,threshold_char);
    strcat(file_name_img,".ppm");
    fpt = fopen(file_name_img,"wb");
    fprintf(fpt,"P5 %d %d 255\n",COLS,ROWS);
    fwrite(thinned_image,COLS*ROWS,1,fpt);
    fclose(fpt);


    printf("image loaded\n");
}