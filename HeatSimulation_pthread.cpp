//most updated pthread program
#include <stdio.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define         X_RESN  200      /* x resolution */
#define         Y_RESN  200      /* y resolution */
#define         total_times 500000
#define         update_freq 400
#define         EPSILON 0.001
#define         no_threads 1

void   printInfo(double runningTime); 

pthread_mutex_t mutex1;
int global_idx;
int avg_row;
int size = X_RESN*Y_RESN;
double *old_temp = new double [size]; 
double *new_temp = new double [size];
double global_diff;

void *slave(void *ignored){

	int local_idx, end, current_pixel;
	double local_diff, temp_after, new_diff; 

	pthread_mutex_lock(&mutex1);
	local_idx = global_idx;
	global_idx += avg_row;
	pthread_mutex_unlock(&mutex1);

	end = (local_idx+avg_row < Y_RESN-1) ? 
		  (local_idx+avg_row) : (Y_RESN-1); 

    local_diff = 0;
    for (int j = local_idx; j < end; j++) 
    for (int i = 1; i < X_RESN-1; i++)
    {
    	current_pixel = j*X_RESN + i;
	    temp_after = (old_temp[current_pixel-1] + old_temp[current_pixel+1] +
                      old_temp[current_pixel-X_RESN] + old_temp[current_pixel+X_RESN])/4;
	    new_diff = fabs(temp_after - old_temp[current_pixel]);
	    new_temp[current_pixel] = temp_after;

        if(new_diff > local_diff) {
            local_diff = new_diff;
        }

    }
    pthread_mutex_lock(&mutex1);
    if (local_diff > global_diff) {
        global_diff = local_diff; 
    }
    pthread_mutex_unlock(&mutex1);
}

int main() {

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
	pthread_t thread[no_threads];
 
    int current_pixel, state, its;
    its = 0;
    avg_row = ceil(double(Y_RESN-2) / no_threads); 

    /* initialize arrays */ 
    for(int i = 0; i < size; i++) {
        new_temp[i] = 0;
    }

    for(int i = 0; i < X_RESN; i++) {
        new_temp[i] = 20;
        new_temp[(X_RESN-1)*X_RESN+i] = 20;
        new_temp[(i*X_RESN)] = 20;
        new_temp[(i+1)*X_RESN - 1] = 20;
    }


    int fireplace_width = X_RESN/8; 
    for(int i = 0; i < fireplace_width; i++) {
        new_temp[X_RESN/2 - i] = 100;
        new_temp[X_RESN/2 + i] = 100;
    }

    while (its < total_times) {
    	// printf("its = %d\n", its);
        global_diff = 0;
        global_idx = 1;
        for (int i = 0; i < size; i++) {
	        old_temp[i] = new_temp[i];
        }

         for(int i = 0; i < size; i++) {
	         printf("%-10f", old_temp[i]);
	         if ((i+1)%X_RESN==0) printf("\n");
	     }
        
         if (its % update_freq == 0) {
             for(int i = 0; i < size; i++) {
                 state = new_temp[i]/5; 
                 XSetForeground (display, gc, color_array[state].pixel);
                 XDrawPoint(display, win, gc, i%X_RESN, i/X_RESN);
                 // printf("");     
             }

         }

        for (int i = 0; i < no_threads; i++) {
			if (pthread_create(&thread[i], NULL, slave, NULL) != 0)
				perror("Pthread_create fails");
		}
		for (int i = 0; i < no_threads; i++) {
			if (pthread_join(thread[i], NULL) != 0)
				perror("Pthread_join fails");
		}

        if (global_diff <= EPSILON) {
            break;
        }
        // printf("global_diff = %f\n", global_diff);
        its++;
    }

    printf("its = %d\n", its);
    gettimeofday(&t_end, NULL);
    double runningTime = (t_end.tv_sec - t_start.tv_sec) + 
                     (t_end.tv_usec - t_start.tv_usec) / 1000000.0;
    printInfo(runningTime);

}

void printInfo(double runningTime) {
    printf("\nName: %s\n", "Liang Jialu");
    printf("Student ID: %d\n", 118010164);
    printf("Assignment 3, N Body, Pthread implementation.\n");
    printf("runTime is %f\n", runningTime);
    printf("number of threads = %d", no_threads);
}
