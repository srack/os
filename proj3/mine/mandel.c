/* Samantha Rack
 * CSE 30341
 * Project 3
 */

#include "bitmap.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#include <pthread.h>

struct ciArgs {
	struct bitmap *bm;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	int maxIter;
	int startY;
	int rowsToCompute;
};

int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
void *compute_image(void *a);

void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates. (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=500)\n");
	printf("-H <pixels> Height of the image in pixels. (default=500)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-n <threads> Number of threads to compute the image. (default=1)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

int main( int argc, char *argv[] )
{
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.

	const char *outfile = "mandel.bmp";
	double xcenter = 0;
	double ycenter = 0;
	double scale = 4;
	int    image_width = 500;
	int    image_height = 500;
	int    max = 1000;
	int numThreads = 1;

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:h:n:"))!=-1) {
		switch(c) {
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				scale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'h':
				show_help();
				exit(1);
				break;
			case 'n':
				numThreads = atoi(optarg);
				if (numThreads <= 0) {
					numThreads = 1;
				}
				break;
		}
	}

	// Display the configuration of the image.
	//printf("mandel: x=%lf y=%lf scale=%lf max=%d outfile=%s\n",xcenter,ycenter,scale,max,outfile);

	// Create a bitmap of the appropriate size.
	struct bitmap *bm = bitmap_create(image_width,image_height);

	// Fill it with a dark blue, for debugging
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

	// Compute the Mandelbrot image
	struct ciArgs *args[numThreads];
	pthread_t tIds[numThreads];
	pthread_attr_t attrs[numThreads];
	
	int i;
	if (numThreads == 1) {
		// allocate memory for the arguments
		args[0] = malloc(sizeof(struct ciArgs));
		if (args[0] == NULL) {
			printf("mandel: malloc: %s\n", strerror(errno));
			exit(1);
		}

		// fill in the structure
		(args[0])->bm = bm;
		(args[0])->xmin = (xcenter-scale);
		(args[0])->xmax = (xcenter+scale);
		(args[0])->ymin = (ycenter-scale);
		(args[0])->ymax = (ycenter+scale);
		(args[0])->maxIter = max;
		(args[0])->startY = 0; 
		(args[0])->rowsToCompute = (int)(bitmap_height(bm));

		compute_image((void*)args[0]);

		free(args[0]);
	}
	else {	
		// start all of the threads	
		for (i = 0; i < numThreads; ++i) {
			// allocate memory for the arguments
			args[i] = malloc(sizeof(struct ciArgs));
			if (args[i] == NULL) {
				printf("mandel: malloc: %s\n", strerror(errno));
				exit(1);
			}
	
			//(args[i])->i = i;

			// fill in the structure
			(args[i])->bm = bm;
			(args[i])->xmin = (xcenter-scale);
			(args[i])->xmax = (xcenter+scale);
			(args[i])->ymin = (ycenter-scale);
			(args[i])->ymax = (ycenter+scale);
			(args[i])->maxIter = max;
			(args[i])->startY = (bitmap_height(bm)/numThreads)*i;
			(args[i])->rowsToCompute = (int)(bitmap_height(bm)/numThreads)*(i+1) - (args[i])->startY;

			// get default attributes
			if(pthread_attr_init(&(attrs[i])) < 0) {
				printf("mandel: pthread_attr_init: %s\n", strerror(errno));
				exit(1);
			}
	
			//create the thread
			if(pthread_create(&(tIds[i]), &(attrs[i]), compute_image, (void *)args[i]) != 0) {
				printf("mandel: pthread_create: %s\n", strerror(errno));
				exit(1);
			} 
		
		}

		// wait for threads to complete and free the arguments
		for (i = 0; i < numThreads; ++i) {
			// wait for the thread
			pthread_join(tIds[i], NULL);
			// free the arguments
			free(args[i]);
		}

	}



	// Save the image in the stated file.
	if(!bitmap_save(bm,outfile)) {
		fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
		return 1;
	}

	return 0;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

void *compute_image(void* a)
{
	int i,j;

	// extract arguments from the structure
	struct bitmap *bm = ((struct ciArgs *)a)->bm;
	double xmin = ((struct ciArgs *)a)->xmin;
	double xmax = ((struct ciArgs *)a)->xmax;
	double ymin = ((struct ciArgs *)a)->ymin;
	double ymax = ((struct ciArgs *)a)->ymax;
	int max = ((struct ciArgs *)a)->maxIter;
	int startY = ((struct ciArgs *)a)->startY;
	int rowsToCompute = ((struct ciArgs *)a)->rowsToCompute;

	int width = bitmap_width(bm);
	int height = bitmap_height(bm);

	// For every pixel in the image...

	for(j=startY;j<startY+rowsToCompute;j++) {

		for(i=0;i<width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = xmin + i*(xmax-xmin)/width;
			double y = ymin + j*(ymax-ymin)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,max);

			// Set the pixel in the bitmap.
			bitmap_set(bm,i,j,iters);
		}
	}

	return 0;
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iteration_to_color(iter,max);
}

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/

int iteration_to_color( int i, int max )
{
	int gray = 255*i/max;
	return MAKE_RGBA(0,gray,gray,0);
}




