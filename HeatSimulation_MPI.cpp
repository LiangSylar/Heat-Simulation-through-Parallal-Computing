#include "mpi.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define         X_RESN  200      /* x resolution */
#define         Y_RESN  200      /* y resolution */
#define         total_times 500000
#define         update_freq 400
#define         EPSILON 0.001
#define         MASTER  0

void printInfo(double runningTime);

int main (int argc, char* argv[])
{
    MPI_Init(&argc, & argv);
    int taskid, 
        numtasks, 
        len;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(hostname, &len);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);


    int size = X_RESN*Y_RESN;
    int avg_row = int(ceil(double(Y_RESN) / (numtasks-1))); 
    int row_num[numtasks-1]; 
    for(int i = 0; i < numtasks-2; i++) {
        row_num[i] = avg_row;
    }
    row_num[numtasks-2] = (Y_RESN%avg_row == 0) ? avg_row : (Y_RESN%avg_row);

    // for(int i = 0; i < numtasks-1; i++) {
    //     printf("slave %d has %d tasks \n", i+1, row_num[i]);
    // }

    if (taskid == MASTER) {
        Window          win;                            /* initialization for a window */
        unsigned
        int             width, height,                  /* window size */
                        x, y,                           /* window position */
                        border_width,                   /*border width in pixels */
                        display_width, display_height,  /* size of screen */
                        screen;                         /* which screen */

        char            *window_name = "Heat Simulation", *display_name = NULL;
        GC              gc;
        unsigned
        long            valuemask = 0;
        XGCValues       values;
        Display         *display;
        XSizeHints      size_hints;
        Pixmap          bitmap;
        XPoint          points[800];
        FILE            *fp, *fopen ();
        char            str[100];
        
        XSetWindowAttributes attr[1];
        timeval t_start, t_end;

        /* connect to Xserver */

        if (  (display = XOpenDisplay (display_name)) == NULL ) {
           fprintf (stderr, "drawon: cannot connect to X server %s\n",
                                XDisplayName (display_name) );
            exit (-1);  
        }
        
        /* get screen size */

        screen = DefaultScreen (display);
        display_width = DisplayWidth (display, screen);
        display_height = DisplayHeight (display, screen);

        /* set window size */

        width = X_RESN;
        height = Y_RESN;

        /* set window position */

        x = 0;
        y = 0;

        /* create opaque window */

        border_width = 4;
        win = XCreateSimpleWindow (display, RootWindow (display, screen),
                                x, y, width, height, border_width, 
                                BlackPixel (display, screen), WhitePixel (display, screen));

        size_hints.flags = USPosition|USSize;
        size_hints.x = x;
        size_hints.y = y;
        size_hints.width = width;
        size_hints.height = height;
        size_hints.min_width = 300;
        size_hints.min_height = 300;
        
        XSetNormalHints (display, win, &size_hints);
        XStoreName(display, win, window_name);

        /* create graphics context */
        attr[0].backing_store = Always;
        attr[0].backing_planes = 1;
        attr[0].backing_pixel = BlackPixel(display, screen);

        XChangeWindowAttributes(display, win, CWBackingStore | CWBackingPlanes | CWBackingPixel, attr);

        XMapWindow (display, win);
        XSync(display, 0);

         /* create colors */ 
        XColor color_array [21];
        XColor color;
        for(int i = 0; i < 4; i++) {
            color.red = color.green = 0;
            color.blue = 65000 - i*10000;
            color_array[i] = color;
            XAllocColor(display, DefaultColormap(display, screen), &color_array[i]);
        }  
        for(int i = 3; i < 15; i++) {
            color.red = color.blue = 0;
            color.green = 10000 + (i-2)*4000;
            color_array[i] = color;
            XAllocColor(display, DefaultColormap(display, screen), &color_array[i]);
        }
        for(int i = 15; i < 21; i++) {
            color.green = color.blue = 0;
            color.red = 30000 + (i-14)*5000;
            color_array[i] = color;
            XAllocColor(display, DefaultColormap(display, screen), &color_array[i]);
        }


        gc = XCreateGC (display, win, valuemask, &values);
        Status rc1 = XAllocColor(display, DefaultColormap(display, screen), &color);
        
        XSetForeground (display, gc, color.pixel);
        XSetBackground (display, gc, WhitePixel (display, screen));

        XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);

        gettimeofday(&t_start, NULL);

        MPI_Status status;
        int Rsize, state, its, current_pixel;
        double Global_diff, Sdiff;
        double *link, *diff_link; 
        double *temp = new double [size];             
        double *Rbuff = new double[size];
        // double *diff_array = new double[numtasks-1];
        double Rdiff;
        its = 0;  

        while (its < total_times) {
            Rdiff = 0;
            link = temp;
            Global_diff = 0;
            for(int i = 1; i < numtasks; i++) {
                Rsize = row_num[i-1]*X_RESN;
                MPI_Recv(link, Rsize , MPI_DOUBLE, i, its, MPI_COMM_WORLD, &status);
                link += Rsize;
            }
                        
             if (its % update_freq == 0) {
                 for(int i = 0; i < size; i++) {
                     state = temp[i]/5; 
                     XSetForeground (display, gc, color_array[state].pixel);
                     XDrawPoint(display, win, gc, i%X_RESN, i/X_RESN);
                     // printf("");     

                 }

            }
            // MPI_Reduce(&Sdiff, &Global_diff, 1, MPI_DOUBLE, MPI_MAX, MASTER, MPI_COMM_WORLD);
            
            for(int i = 1; i < numtasks; i++) {
                MPI_Recv(&Rdiff, 1, MPI_DOUBLE, i, its, MPI_COMM_WORLD, &status);
                if (Rdiff > Global_diff) {
                    Global_diff = Rdiff;
                }                              
            }
            if (Global_diff <= EPSILON) {
                break;
            }
            its++;
        }

        printf("its = %d\n", its);
        gettimeofday(&t_end, NULL);
        double runningTime = (t_end.tv_sec - t_start.tv_sec) + 
                         (t_end.tv_usec - t_start.tv_usec) / 1000000.0;
        printInfo(runningTime);
        printf("nunmber of cores is %d\n", numtasks);
    }

    else {
        MPI_Status status;         
        int current_pixel, Sits, Srow, Ssize;
        Sits = 0;
        Srow = row_num[taskid-1];
        Ssize = Srow*Y_RESN;

        double *old_temp = new double [Ssize]; 
        double *new_temp = new double [Ssize];   
        double Snew_diff, Sdiff;
        double up_row[X_RESN];
        double down_row[X_RESN];
        double *Slink;
        Slink = new_temp;
        Slink += Y_RESN*(Srow-1);

        /* initialize arrays */
        for(int i = 0; i < Ssize; i++) {
            new_temp[i] = 0;   
        }

        for(int i = 0; i < Srow; i++) {
            new_temp[(i*X_RESN)] = 20;
            new_temp[(i+1)*X_RESN - 1] = 20;
        }

        if (taskid == 1) { 
            int fireplace_width = X_RESN/8;
            for(int i = 0; i < X_RESN; i++) {
                new_temp[i] = 20;
            } 
            for(int i = 0; i < fireplace_width; i++) {
                new_temp[X_RESN/2 - i] = 100;
                new_temp[X_RESN/2 + i] = 100;
            }

        }
        if (taskid == numtasks-1) {
            for(int i = 0; i < X_RESN; i++) {
                new_temp[Ssize-i] = 20;
            }
        }

        while (Sits < total_times) {

            Sdiff = 0; 

            for (int i = 0; i < Ssize; i++) {
                old_temp[i] = new_temp[i];
            }

            if (taskid < numtasks - 1) { /* send last row to next processor */
                MPI_Recv(down_row, X_RESN , MPI_DOUBLE, taskid+1, Sits, MPI_COMM_WORLD, &status);
                MPI_Send(Slink, X_RESN, MPI_DOUBLE, taskid+1, Sits, MPI_COMM_WORLD);
            }
            if (taskid >= 2) { /* send first row to previous processor */
                MPI_Send(new_temp, X_RESN, MPI_DOUBLE, taskid-1, Sits, MPI_COMM_WORLD);
                MPI_Recv(up_row, X_RESN , MPI_DOUBLE, taskid-1, Sits, MPI_COMM_WORLD, &status);                 
            }


            for (int j = 1; j < Srow-1; j++) 
            for (int i = 1; i < X_RESN-1; i++)
            {
                current_pixel = j*X_RESN + i;
                new_temp[current_pixel] = (old_temp[current_pixel-1] + old_temp[current_pixel+1] +
                               old_temp[current_pixel-X_RESN] + old_temp[current_pixel+X_RESN])/4;
                Snew_diff = fabs(new_temp[current_pixel]-old_temp[current_pixel]);
                if (Snew_diff > Sdiff) {
                    Sdiff = Snew_diff; 
                }
            }

            if (taskid < numtasks - 1) { /* compute new_temp for the last row */
                for(int i = 1; i < X_RESN-1; i++) {
                    current_pixel = (Srow-1)*X_RESN + i;
                    new_temp[current_pixel] = (old_temp[current_pixel-1] + old_temp[current_pixel+1] +
                            down_row[i] + old_temp[current_pixel-X_RESN])/4;
                    Snew_diff = fabs(new_temp[current_pixel]-old_temp[current_pixel]);
                }
                if (Snew_diff > Sdiff) {
                    Sdiff = Snew_diff; 
                }
            }
            if (taskid > 1) { /* compute new_temp for the first row */
                for(int i = 1; i < X_RESN-1; i++) {
                    new_temp[i] = (old_temp[i-1] + old_temp[i+1] +
                            up_row[i] + old_temp[i+X_RESN])/4;
                    Snew_diff = fabs(new_temp[i]-old_temp[i]);
                }
                if (Snew_diff > Sdiff) {
                    Sdiff = Snew_diff; 
                }
            }

            // MPI_Reduce(&Sdiff, &Global_diff, 1, MPI_DOUBLE, MPI_MAX, MASTER, MPI_COMM_WORLD);
            MPI_Send(new_temp, Ssize, MPI_DOUBLE, MASTER, Sits, MPI_COMM_WORLD); 
            MPI_Send(&Sdiff, 1, MPI_DOUBLE, MASTER, Sits, MPI_COMM_WORLD); 

            Sits++;
        }

    }
    // MPI_Finalize();
    return 0;
}

void printInfo(double runningTime) {
    printf("\nName: %s\n", "Liang Jialu");
    printf("Student ID: %d\n", 118010164);
    printf("Assignment 4, Heat Simulation, MPI implementation.\n");
    printf("runTime is %f\n", runningTime);
}
