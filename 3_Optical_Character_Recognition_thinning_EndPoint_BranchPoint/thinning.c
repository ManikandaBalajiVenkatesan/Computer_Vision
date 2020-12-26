#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main (int argc, char *argv[])
{
    FILE *fpt;
    unsigned char *image, *msf_image, *detection;   //input image and msf image, temporary image for threshold purposes
    unsigned char *letter_image;          //image to store the cropped letter box 
    unsigned char *template;        //to store template image
    char		header[320];
    int		i_ROWS,i_COLS,i_BYTES;  //parameters of the image
    int T;  //threshold value
    int detected;   //to say whether letter has been detected
    char letter;    //variable to store letter from ground truth data
    int tr, tc;     //position of letter , from ground truth data
    int     t_ROWS,t_COLS,t_BYTES;  //parameters of the template
    int l_ROWS, l_COLS;             //size of the letter image with padding
    int transition; //to record transition from edge to non edge for thinning
    int neighbors;  //to record number of edge neighbors around every edge pixle
    int NEWS;       //to record the north or east or (west and south) condition
    unsigned char *erase_selection;
    int deletion;   //number of pixels to be erased for thinning
    int end_points=0, branch_points=0;   //number of edge and branch points 
    char d_letter = 'e';            //letter to be detected

    //variables to store for ROC analysis
    int TP=0, FP=0, FN=0, TN=0;
    float TPR=0, FPR=0, PPV=0, sensitivity = 0, specificity=0;


    //to open the source image
    fpt = fopen("parenthood.ppm","rb");
    if (fpt == NULL)
        printf("check for parenthood.ppm\n");
    
    int i = fscanf(fpt,"%s %d %d %d ",header,&i_COLS,&i_ROWS,&i_BYTES);
    image=(unsigned char *)calloc(i_ROWS*i_COLS,sizeof(unsigned char));
    fread(image,1,i_COLS*i_ROWS,fpt);
    fclose(fpt);

    //to open msf image
    fpt = fopen("msf_e.ppm","rb");
    if (fpt == NULL)
        printf("check for msf_e.ppm\n");
    
    i = fscanf(fpt,"%s %d %d %d ",header,&i_COLS,&i_ROWS,&i_BYTES);
    msf_image=(unsigned char *)calloc(i_ROWS*i_COLS,sizeof(unsigned char));
    fread(msf_image,1,i_COLS*i_ROWS,fpt);
    fclose(fpt);

    //to open the template image
    fpt = fopen("parenthood_e_template.ppm","rb");
    if (fpt == NULL)
        printf("check for parenthood_e_template.ppm\n");

    i = fscanf(fpt,"%s %d %d %d ",header,&t_COLS,&t_ROWS,&t_BYTES);
    template = (unsigned char *)calloc(t_ROWS*t_COLS,sizeof(unsigned char));
    fread(template,1,t_COLS*t_ROWS,fpt);
    fclose(fpt);
    l_ROWS = t_ROWS+2; l_COLS= t_COLS+2;//since we are have to check edge to non edge transition around every pixel we increase the size by 2

    float TP_array[i_BYTES+1], FP_array[i_BYTES+1];
    float FN_array[i_BYTES+1], TN_array[i_BYTES+1];
    float TPR_array[i_BYTES+1], FPR_array[i_BYTES+1];
    float PPV_array[i_BYTES+1], T_array[i_BYTES+1];
    float sensitivity_array[i_BYTES+1], specificity_array[i_BYTES+1];

    for(T=0; T<=255; T++)
    {
        TP=0; FP=0; FN=0; TN=0;
        
        detection = (unsigned char *)calloc(i_ROWS*i_COLS,sizeof(unsigned char));
        for (int i=0; i<i_ROWS*i_COLS; i++)
        {
            if (msf_image[i] >= T)
                detection[i] = 255;
            else
                detection[i] = 0;
        }

        fpt = fopen("ground_truth.txt","r");
        if (fpt == NULL)
            printf("check for ground_truth.txt\n");
        while(1)
        {
            i = fscanf(fpt, "%c %d %d ",&letter, &tc, &tr);
            if (i != 3)
                break;
            detected = 0;
            
            //the detected pixel may not be at the same value always therefore we check in the box of template size
            for (int r=-t_ROWS/2; r<=t_ROWS/2; r++)
                for(int c=-t_COLS/2; c<=t_COLS/2; c++)
                    if (detection[(r+tr)*i_COLS + c+tc] == 255)
                        detected = 1;
            
            if (detected == 1)
            {
                letter_image = (unsigned char *)calloc(l_ROWS*l_COLS,sizeof(unsigned char));
                for (int r=-l_ROWS/2+1; r<=l_ROWS/2-1; r++)
                    for(int c=-l_COLS/2+1; c<=l_COLS/2-1; c++)
                        letter_image[(r+l_ROWS/2)*l_COLS + (c+l_COLS/2)] = image[(tr+r)*i_COLS + (tc+c)];  
                
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


                 /* 
                // printing letter pixels
                printf("letter image binary l_rows * l_cols  %d i %d\n", l_ROWS*l_COLS, i);
                for(int r=0; r<l_ROWS; r++)
                {
                    for(int c=0; c<l_COLS; c++)
                            if (letter_image[r*l_COLS+c] == 1)
                                printf("255\t");
                            else
                                printf("0\t");
                    printf("\n");
                }
                */
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

                    /*
                    printf("after thinning %d\n", iteration);
                    for(int r=0; r<l_ROWS; r++)
                    {
                        for(int c=0; c<l_COLS; c++)
                            if (letter_image[r*l_COLS+c] == 1)
                                printf("255\t");
                            else
                                printf("0\t");
                        printf("\n");
                    }
                    printf("\n\n\n\n");
                    */
             
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
            }
            //printf("e %d b %d r %d c %d\n",end_points, branch_points, tr, tc);
            if (end_points != 1 && branch_points != 1 )
                    detected = 0;
            //printf("end points %d branch points is %d\n",end_points, branch_points);
            //printf("detection value %d\n",detected);
            //printf("%c r %d c %d d %d end %d branch %d\n",letter, tr, tc, detected, end_points, branch_points);
            if (letter == d_letter && detected == 1)
                TP+=1;
            if (letter == d_letter && detected == 0)
                FN+=1;
            if (letter != d_letter && detected == 1)
                FP+=1;
            if (letter != d_letter && detected == 0)
                TN+=1; 
                
        }
        
        fclose(fpt);
        // printf("%d\t%d\t%d\t%d\t%d\n",T,TP,FN,FP,TN);
        
        TPR = (float)TP/(TP+FN);
        FPR = (float)FP / (FP + TN);
        PPV = (float)FP / (TP + FP);
        sensitivity = TPR;
        specificity = (float)1 - FPR;
        T_array[T] = T; TP_array[T] = TP; FN_array[T] = FN;
        FP_array[T] = FP; TN_array[T] = TN;
        TPR_array[T] = TPR; FPR_array[T] = FPR; PPV_array[T] = PPV;
        sensitivity_array[T] = sensitivity;
        specificity_array[T] = specificity;
        
    }
    //exporting data for ROC graph
    fpt = fopen("ROC_curve_data","w");
    fprintf(fpt, "T\tTPR\t\tFPR\n");
    for(int T=0; T<=255; T++)
        fprintf(fpt,"%d\t%f\t\t%f\n",T, TPR_array[T], FPR_array[T]);
    fclose(fpt);


}