/* Sequential Mandelbrot program */
#include <omp.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define         X_RESN  200 /* x resolution */
#define         Y_RESN  200      /* y resolution */
#define         total_times 100000
#define         update_freq 400
#define         EPSILON 0.001
#define         no_threads 4
                            4
void printInfo(double runningTime);

int main ()
{   


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
    // add pragma here 
    for(int i = 0; i < 4; i++) {
        color.red = color.green = 0;
        color.blue = 65000 - i*10000;
        color_array[i] = color;
        XAllocColor(display, DefaultColormap(display, screen), &color_array[i]);
    }  
    // add pragma here 
    for(int i = 3; i < 15; i++) {
        color.red = color.blue = 0;
        color.green = 10000 + (i-2)*4000;
        color_array[i] = color;
        XAllocColor(display, DefaultColormap(display, screen), &color_array[i]);
    }
    // add pragma here 
    for(int i = 15; i < 21; i++) {
        color.green = color.blue = 0;
        color.red = 30000 + (i-14)*5000;
        color_array[i] = color;
        XAllocColor(display, DefaultColormap(display, screen), &color_array[i]);
    }

    gc = XCreateGC (display, win, valuemask, &values);
    XSetLineAttributes (display, gc, 1, LineSolid, CapRound, JoinRound);

   /* create arrays for pixels' temprature */
    int size = X_RESN*Y_RESN;
    double *old_temp = new double [size]; 
    double *new_temp = new double [size];

    gettimeofday(&t_start, NULL);

    double diff, new_diff;
    double diff_array[size];
    int current_pixel, state;
    int its = 0;

    /* set initial temperature */
    #pragma omp parallel for num_threads(no_threads) 
    for(int i = 0; i < size; i++) {
        new_temp[i] = 0;
        diff_array[i] = 0;
        // new_temp[i] = 0;
    }

    // add pragma here 
    // #pragma omp parallel for num_threads(no_threads) 
    for(int i = 0; i < X_RESN; i++) {
        new_temp[i] = 20;
        new_temp[(X_RESN-1)*X_RESN+i] = 20;
        new_temp[(i*X_RESN)] = 20;
        new_temp[(i+1)*X_RESN - 1] = 20;
    }


    int fireplace_width = X_RESN/10;
    // add pragma here  
    for(int i = 0; i < fireplace_width; i++) {
        new_temp[X_RESN/2 - i] = 100;
        new_temp[X_RESN/2 + i] = 100;
    }

    while (its < total_times) {

        // add pragma here 
        #pragma omp parallel for num_threads(no_threads)
        for (int i = 0; i < size; i++) {
            old_temp[i] = new_temp[i];
        }
        
        if (its % update_freq == 0) {
            for(int i = 0; i < size; i++) {

                state = new_temp[i]/5; 
                XSetForeground (display, gc, color_array[state].pixel);
                XDrawPoint(display, win, gc, i%X_RESN, i/X_RESN);
                // printf("");     

            }

        }

        // add pragma here 
        #pragma omp parallel for num_threads(no_threads) private(current_pixel, new_diff, diff)
        for (int j = 1; j < X_RESN-1; j++) 
        for (int i = 1; i < X_RESN-1; i++)
        {
            current_pixel = j*X_RESN + i;
            new_temp[current_pixel] = (old_temp[current_pixel-1] + old_temp[current_pixel+1] +
                           old_temp[current_pixel-X_RESN] + old_temp[current_pixel+X_RESN])/4;
            new_diff = fabs(new_temp[current_pixel]-old_temp[current_pixel]);
            diff_array[current_pixel] = new_diff;
        }


        diff = diff_array[1];
        for(int i = 2; i < size-1; i++) {
            diff = (diff_array[i] > diff )? diff_array[i]:diff;
        }
        // printf("%f\n", diff);
        if (diff <= EPSILON) break;
        its++;

    }        
    // printf("its = %d\n", its);
    gettimeofday(&t_end, NULL);
    double runningTime = (t_end.tv_sec - t_start.tv_sec) + 
                     (t_end.tv_usec - t_start.tv_usec) / 1000000.0;
    printInfo(runningTime);
    printf("number of no_threads is %d\n", no_threads);

    /* Program Finished */
    return 0;
}

void printInfo(double runningTime) {
    printf("\nName: %s\n", "Liang Jialu");
    printf("Student ID: %d\n", 118010164);
    printf("Assignment 4, Heat Simulation, openMP implementation.\n");
    printf("runTime is %f\n", runningTime);
    // printf("number of no_threads is %d\n", no_threads);
}
