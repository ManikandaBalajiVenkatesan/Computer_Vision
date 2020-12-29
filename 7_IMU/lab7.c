#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define SQR(x) ((x)*(x));


//global variables
int total_data;         //total number of data points
int freq = 20;          //frequency of measurement
double Ts;   //sampling time of measurement
int window_f_g = 13;    //window size of the moving average filter gyro
int window_f_a = 13;    //window size of the moving average filter acc
int window_v = 13;    //window size for variance calculation
double threshold_l_l = 0.00001;   //lower threshold for linear movement
double threshold_h_l = 0.005;   //higher threshold for linear movement
double threshold_r = 0.01;  //threshold for rotational movement

void moving_average(double *raw, double* filtered, int window)
{
    int count = 0;
    double average = 0.0;
    while(count<total_data)
    {
        average = 0.0;
        if (count<(window-1))
        {
            *(filtered+count) = *(raw+count);
            count++;
            continue;
        }
        for(int i = 0;i<window; i++)
            average = average + *(raw+count-i);
        average = average/window;
        *(filtered+count) = average;
        count++;
    }
}

void variance_calculator(double *data, double *var_data)
{
    int count = 0;
    double average = 0.0, variance = 0.0;
    while(count<total_data)
    {
        average = 0.0;
        if (count<(window_v-1))
        {
            *(var_data+count) = *(data+count);
            count++;
            continue;
        }
        
        //average calculation
        for(int i = 0;i<window_v; i++)
            average = average + *(data+count-i);
        average = average/window_v;
        //variance calculation
        for(int i = 0; i<window_v; i++)
            variance = variance + SQR(*(data+count-i) - average); 
        variance = variance / (window_v-1);
        *(var_data+count) = variance;
        count++;
    }
}

double rotation_calculator(double *gyro_data, double *gyro_var, char data_name[10], double *rot_data)
{
    FILE *fpt;
    fpt=fopen(data_name,"w");
    fprintf(fpt,"Time(s)\t\tData(rad/s)\tVariance\tRotate\tAngle(deg)\n");
    double rotation = 0.0;
    bool movement = false;
    for(int i = 0;i<total_data; i++)
    {
        movement = false;
        if(fabs(*(gyro_var+i)) > threshold_r)
        {
            //printf("%f %lf %lf \n",i*Ts,*(gyro_var+i),*(gyro_data+i));
            movement = true;
            rotation = rotation + *(gyro_data+i)*Ts*180/M_PI;
        }
        fprintf(fpt,"%lf\t%lf\t%lf\t%d\t%lf\n",i*Ts,*(gyro_data+i),*(gyro_var+i),movement,rotation);
        *(rot_data+i) = rotation;
    }
    fclose(fpt);
    return rotation;
}


double motion_calculator(double *acc_data, double *acc_var, double *gyro_var, char data_name[10], double *disp_data)
{

    double disp = 0.0, vel_curr = 0.0, vel_prev = 0.0, avg_vel = 0.0, acc = 0.0;
    bool movement = false;
    FILE *fpt;
    fpt = fopen(data_name,"w");
    fprintf(fpt,"Time(s)\tAcc(m/s2)\tV (m/s)\tV_avg (m/s)\tVar\tgyro_var\tMovement\tDisplacement(m)\n");
    for(int i = 0; i<total_data; i++)
    {
        movement = false;
        vel_curr = 0.0;
        acc = *(acc_data + i);
        // printf("Acc %lf\n",acc);
        
        if(fabs(*(acc_var + i)) > threshold_l_l && fabs(*(acc_var + i)) < threshold_h_l && *(gyro_var+i) < threshold_r)
        {
            vel_curr = vel_prev + acc*Ts;
            avg_vel = (vel_curr + vel_prev)/2;
            disp = disp + (avg_vel * Ts);
            movement = true;
        }           
        vel_prev = vel_curr;
        *(disp_data+i) = disp;
        fprintf(fpt,"%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%d\t%lf\n",i*Ts,acc,vel_curr,avg_vel,*(acc_var + i),*(gyro_var+i),movement,disp); 
    } 
    fclose(fpt);
    return disp;
}

int main (int argc, char* argv[])
{
    FILE *fpt;
    total_data = 0;     //count of data points
    double acc_x_r[1250], acc_y_r[1250], acc_z_r[1250];   //raw accelerometer measurements
    double pitch_r[1250], roll_r[1250], yaw_r[1250];     //raw gyroscope measurements
    double acc_x[1250], acc_y[1250], acc_z[1250];           //filtered accelerometer measurements
    double pitch[1250], roll[1250], yaw[1250];          //filtered gyroscope measurements
    double acc_x_var[1250], acc_y_var[1250], acc_z_var[1250];           //variance accelerometer filtered measurements
    double pitch_var[1250], roll_var[1250], yaw_var[1250];          //variance gyroscope filtered measurements
    double rot_pitch[1250], rot_roll[1250], rot_yaw[1250];          //variance gyroscope filtered measurements
    double time_stamp[1250];                            //variable to store time stamp of the data
    int i,count = 0;
    double disp_x[1250], disp_y[1250], disp_z[1250];       //motion in x, y, z axis

    Ts = 1/(double)freq;     //time step in seconds
    //importing raw data
    fpt = fopen("lab7_input.txt","r");
    
    while(1)
    {
        i=fscanf(fpt,"%lf %lf %lf %lf %lf %lf %lf ",&time_stamp[count], &acc_x_r[count], &acc_y_r[count], &acc_z_r[count], &pitch_r[count], &roll_r[count], &yaw_r[count]);
        if(i < 1)
            break;
        count++;
    }
    total_data = count;
    fclose(fpt);

    //data filteration to smoothen out the data - moving average filter
    moving_average(acc_x_r, acc_x,window_f_a);
    moving_average(acc_y_r, acc_y,window_f_a);
    moving_average(acc_z_r, acc_z,window_f_a);
    moving_average(pitch_r, pitch,window_f_g);
    moving_average(roll_r, roll,window_f_g);
    moving_average(yaw_r, yaw,window_f_g);

    variance_calculator(acc_x, acc_x_var);
    variance_calculator(acc_y, acc_y_var);
    variance_calculator(acc_z, acc_z_var);
    variance_calculator(pitch, pitch_var);
    variance_calculator(roll, roll_var);
    variance_calculator(yaw, yaw_var);


    rotation_calculator(roll, roll_var,"roll.txt",rot_roll);
    rotation_calculator(pitch, pitch_var,"pitch.txt",rot_pitch);
    rotation_calculator(yaw, yaw_var,"yaw.txt",rot_yaw);
  
    motion_calculator(acc_x, acc_x_var, roll_var,"x_direction.txt",disp_x);
    motion_calculator(acc_y, acc_y_var, pitch_var,"y_direction.txt",disp_y);
    motion_calculator(acc_z, acc_z_var, yaw_var,"z_direction.txt",disp_z);

    printf("Prcessed data is in processed_data.txt\n");
    fpt = fopen("processed_data.txt","w");
    fprintf(fpt,"time\tRoll (degrees)\tYaw (degrees)\tPitch (degrees)\tX\tY\tZ\n");
    for(count=0;count<total_data;count++)
        fprintf(fpt,"%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",count*Ts,rot_roll[count],rot_yaw[count],rot_pitch[count],disp_x[count],disp_y[count],disp_z[count]);
    fclose(fpt);
}


/* to save filtered data to file
count = 0;
fpt = fopen("acc_x_filtered.txt","w");
for(count=0;count<total_data;count++)
    fprintf(fpt,"%lf\n",acc_x[count]);
fclose(fpt);
*/