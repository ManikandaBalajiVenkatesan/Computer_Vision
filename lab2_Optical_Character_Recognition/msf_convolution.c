#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main (int argc, char *argv[])
{
    FILE *fpt;
    unsigned char *image, *template, *msf_normalized, *detection, *ground_truth; 
    float *zm_template, *msf_convolved;
    char		header[320];
    int		i_ROWS,i_COLS,i_BYTES;  //parameters of the image
    int     t_ROWS,t_COLS,t_BYTES;  //parameters the template
    int tr,tc;      //tr-target row, tc-target coloumn about which we want to put box
    char letter;    //letter in our ground truth
    char d_letter;  //desired letter 
    float average=0.0;  //average value of template
    float sum;      //for calculation of convolution
    float min=0, max=0;   //for normalization
    float data;
    int wr,wc;  //template dimension
    int pixel;
    int TP=0, FP=0, FN=0, TN=0;
    float TPR=0, FPR=0, PPV=0, sensitivity = 0, specificity=0;
    int detected=0;
    

    if (argc != 3)
    {
        printf("Usage bounding_box [source_image] [letter_template]");
        exit(0);
    }
            
    d_letter = argv[1][0];
    
    //to open the source image
    fpt = fopen(argv[1],"rb");
    if (fpt == NULL)
        printf("check for parenthood.ppm\n");
    
    int i = fscanf(fpt,"%s %d %d %d ",header,&i_COLS,&i_ROWS,&i_BYTES);
    image=(unsigned char *)calloc(i_ROWS*i_COLS,sizeof(unsigned char));
    msf_convolved = (float *)calloc(i_ROWS*i_COLS,sizeof(float));
    msf_normalized=(unsigned char *)calloc(i_ROWS*i_COLS,sizeof(unsigned char));
    ground_truth=(unsigned char *)calloc(i_ROWS*i_COLS,sizeof(unsigned char));
    fread(image,1,i_COLS*i_ROWS,fpt);
    fclose(fpt);

    //to open the template image
    fpt = fopen(argv[2],"rb");
    if (fpt == NULL)
        printf("check for parenthood_e_template.ppm\n");

    i = fscanf(fpt,"%s %d %d %d ",header,&t_COLS,&t_ROWS,&t_BYTES);
    template = (unsigned char *)calloc(t_ROWS*t_COLS,sizeof(unsigned char));
    zm_template = (float *)calloc(t_ROWS*t_COLS,sizeof(float));    //dynamic memory allocation of zero mean template
    fread(template,1,t_COLS*t_ROWS,fpt);
    fclose(fpt);
    wr = t_ROWS - 1;
    wc = t_COLS - 1;

    
    //calculation of average brightness of template
    for(int i=0; i<t_COLS*t_ROWS; i++)
        average+=template[i];
    average = average/(t_COLS*t_ROWS);
    

    //zero mean template
    for(int i=0; i<t_COLS*t_ROWS; i++)
        zm_template[i]= template[i] - average;
        
    
        
    for(int r=0; r<i_ROWS; r++)
        for(int c=0; c<i_COLS; c++)
        {
            sum = 0.0;
            if (r<wr/2 || r >= i_ROWS-wr/2 || c<wc/2 || c>=i_COLS-wc/2)
                sum = 0.0;
            else
            {
            for(int dr=-wr/2; dr<=wr/2; dr++)
                for(int dc=-wc/2; dc<=wc/2; dc++)
                    sum+=(image[(r+dr)*i_COLS + c+dc] * zm_template[(dr+wr/2)*t_COLS + dc+wc/2]);
            }
            //printf("convolved value is %f\n",sum);
            msf_convolved[r*i_COLS + c] = sum;
        }

    max = 0; min = 99999999;
    for(int i=0; i<i_ROWS*i_COLS; i++)
    {
        data = msf_convolved[i];
        //printf("%f\n", data);
        if (data > max)
            max = data;
        if (data < min)
            min = data;    
    }       


    //normalization of convolved image
    int new_max = 255, new_min = 0;
    for(int i=0; i<i_ROWS*i_COLS; i++)
    {
        pixel = round(((msf_convolved[i] - min) * (new_max - new_min) / (max - min)) + new_min);
        msf_normalized[i] = pixel;
    }


    fpt = fopen("normalized_msf.ppm","wb");
    fprintf(fpt,"P5 %d %d 255\n",i_COLS,i_ROWS);
    fwrite(msf_normalized,i_COLS*i_ROWS,1,fpt);
    fclose(fpt);


    fpt = fopen("normalized_msf.ppm","rb");
    if (fpt == NULL)
        printf("check for normalized_msf.ppm\n");
    
    i = fscanf(fpt,"%s %d %d %d ",header,&i_COLS,&i_ROWS,&i_BYTES);
    unsigned char *norm_ref;
    norm_ref=(unsigned char *)calloc(i_ROWS*i_COLS,sizeof(unsigned char));
    fread(norm_ref,1,i_COLS*i_ROWS,fpt);
    fclose(fpt);

    float TP_array[i_BYTES+1], FP_array[i_BYTES+1];
    float FN_array[i_BYTES+1], TN_array[i_BYTES+1];
    float TPR_array[i_BYTES+1], FPR_array[i_BYTES+1];
    float PPV_array[i_BYTES+1], T_array[i_BYTES+1];
    float sensitivity_array[i_BYTES+1], specificity_array[i_BYTES+1];

    for (int T=0; T<=255; T++)
    {
        detection = (unsigned char *)calloc(i_ROWS*i_COLS,sizeof(unsigned char));
        for (int i=0; i<i_ROWS*i_COLS; i++)
        {
            if (msf_normalized[i] >= T)
                detection[i] = 255;
            else
                detection[i] = 0;
        }

        TP=0; FP=0; FN=0; TN=0;
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
            for (int r=tr-wr/2; r<=tr+wr/2; r++)
                for(int c=tc-wc/2; c<=tc+wc/2; c++)
                    if (detection[r*i_COLS + c] == 255)
                        detected = 1;
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
        // printf("%d\t%d\t%d\t%d\n",TP,FN,FP,TN);
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

    fpt = fopen("ROC_curve_data","w");
    fprintf(fpt, "T\tTPR\t\tFPR\n");
    for(int T=0; T<=255; T++)
        fprintf(fpt,"%d\t%f\t\t%f\n",T, TPR_array[T], FPR_array[T]);
    fclose(fpt);

}