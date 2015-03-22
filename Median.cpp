// ============================================================================
// median.cpp - median program.
// Copyright (C) 2015 by Rosario Antunez, Steven Eisinger, and Ethan Graber
//
// Written by: Rosario Antunez, Steven Eisinger, and Ethan Graber, 2015
// ============================================================================

//#include "stdafx.h"
#include "IP.h"
#include <math.h>
using namespace std;

// function prototype
void median(imageP I1, int n1, int n2, imageP I2);
void top_rows_padding(unsigned char* temp_buffer, unsigned char* buffer, uchar	*in, int buffer_width, imageP I1, int MC, int n1);
void median_calc_save(imageP I1, int n1, int top_left_pixel, int buffer_width, unsigned char* buffer, int f, uchar *out, unsigned char* hood);
void next_padded_row(unsigned char* buffer, int buffer_width, imageP I1, int n1, int MC, uchar *in, int bottom_left_pixel, int buffer_size);


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// main:
//
// Main routine to create median image.
//
int main(int argc, char** argv)
{
	int	n1,n2, t1, t2;
	imageP	I1, I2;

	// error checking: proper usage
	if (argc != 5)
	{
		cerr << "Usage: median infile n1 n2 outfile\n";
		exit(1);
	}

	// read input image (I1) and reserve space for output (I2)
	I1 = IP_readImage(argv[1]);
	I2 = NEWIMAGE;

	// read n1 and n2
	n1 = atoi(argv[2]);
	n2 = atoi(argv[3]);
	

	// Apply median and save result in file
	median(I1, n1, n2, I2);
	IP_saveImage(I2, argv[4]);

	// free up image structures/memory
	IP_freeImage(I1);
	IP_freeImage(I2);


	return 1;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// median:
//
// Apply median to I1 and save the output in I2.
// To devide 256 intensities into uniform intervals n = 2^k, where k in 0..8
// The following rules is used for applying median:
// n1 = sz = n x n to create neighborhood size
// n2 = avg_nbrs = (average_above+average_below)/2
void median(imageP I1, int n1, int n2, imageP I2){
	int	 i, j, k, total, h1[MXGRAY], h2[MXGRAY], top_left_pixel, bottom_left_pixel, current_pixel, smaller, larger, num_small, num_large, median_sum = 0, median_value, Nhood = n1*n1;
	uchar	*in, *out;


	// total number of pixels in image
	total = I1->width * I1->height;



	// init I2 dimensions and buffer
	I2->width = I1->width;
	I2->height = I1->height;
	I2->image = (uchar *)malloc(total);
	if (I2->image == NULL) {
		cerr << "qntz: Insufficient memory\n";
		exit(1);
	}

	//set location of image buffers
	in = I1->image;				/* input image buffer */
	out = I2->image;			/* output image buffer */


	//analyize relationship between n1, which represents a nXn matrix and padding called MC Matrix Constant
	int MC = (n1 - 1) / 2;


	//create padded buffer for border implimentation
	//there are 3 cases: top rows, middle rows, and bottom rows
	//top rows: creates padding for the top border of the image
	//bottom rows: creates padding for the bottom border of the image
	//middle rows: creates padding on the left and right borders

	unsigned char* temp_buffer = new unsigned char[I1->width*n1];//creates an array that is used for top and bottom padding which does not pad left or right
	unsigned char* buffer = new unsigned char[(I1->width*n1) + (n1*MC * 2)];//creates an array that is used for left and right, also used as the second step of top and bottom
	int buffer_size = (I1->width*n1) + (n1*MC * 2);
	int buffer_width = buffer_size / n1;
	unsigned char* hood = new unsigned char[Nhood];


	//top row padding
	//done one time during entire program
	top_rows_padding(temp_buffer, buffer, in, buffer_width, I1, MC, n1);


	for (int f = 0; f < I1->height; ++f){					//goes through all the rows
		//use Matrix Constant MC to create top_left_pixel and bottom_left_pixel, which contains the pixel number for the top left corner and bottom left corner of matrix nxn
		for (int g = 0; g < total; g++){
			top_left_pixel = g - MC*(I1->width) - MC;
			bottom_left_pixel = g + MC*(I1->width) - MC;
		}
		//commpute the median for entire row and save it to out[]
		median_calc_save(I1,n1,top_left_pixel, buffer_width,buffer,f,out, hood);

		//delete first row add bottom row and pad new row
		next_padded_row(buffer, buffer_width, I1, n1, MC, in, bottom_left_pixel, buffer_size);

	}
	
}

void top_rows_padding(unsigned char* temp_buffer, unsigned char* buffer, uchar	*in, int buffer_width, imageP I1, int MC, int n1){
	int i, j, k;
	for (j = 0; j < MC + 1; ++j){							//controls how many rows to pad above and includes first row of pixels that were copied
		for (i = 0; i < I1->width; ++i){				//swings left to right across rows
			temp_buffer[i + j*I1->width] = in[i];		//saves results
		}
	}
	for (j = MC + 2; j < n1; ++j){										//copies all the existing rows needed minus the first row
		for (i = 0; i < I1->width; ++i){								//swings left to right across rows
			temp_buffer[i + j*I1->width] = in[i + I1->width*(j - 3)];	//saves results, at this point we have a temp_buffer filled completely that includes padded top rows and the original pixels
		}
	}
	for (j = 0; j < n1; ++j){
		for (i = 0; i < MC; ++i){
			buffer[i + j*buffer_width] = temp_buffer[j*I1->width];						//pads left side
			buffer[i + I1->width + j*buffer_width] = in[j*I1->width - 1];			//pads right side
			for (k = 0; k < I1->width; k++){
				buffer[k + MC + j*buffer_width] = in[k + j*I1->width];					 //copies middle pixels
			}

		}
	}
	delete[] temp_buffer;

}

void median_calc_save(imageP I1, int n1, int top_left_pixel, int buffer_width, unsigned char* buffer, int f, uchar *out, unsigned char* hood){
	int i, j, k, median_sum, median_value, current_pixel;

	

	//populate array with hood values given by our input buffer
	for (k = 0; k < I1->width; ++k){
		for (i = 0; i < n1; ++i){ //row selection
			for (j = 0; j < n1; ++j){ //col selection
				current_pixel = top_left_pixel + (buffer_width)*i + j;
				hood[i + j] = buffer[current_pixel];

			}
		}
		int median_sum = 0;
		for (i = 0; i < n1*n1; ++i){
			median_sum += hood[i];
		}
		median_value = median_sum / (n1*n1);
		out[k + f*I1->width] = median_value;
	}

}

void next_padded_row(unsigned char* buffer, int buffer_width, imageP I1, int n1, int MC, uchar *in, int bottom_left_pixel, int buffer_size){
	int i;
	//the following code uses the buffer which is already padded and adds 1 row and pads it
	//subtract top row and add bottom row
	for (i = 0; i < I1->width*(n1 - 1); ++i)
		buffer[i] = buffer[i + buffer_width];

	//add new line from in[]
	for (i = 0; i < I1->width; ++i)									//swings left to right across rows
		buffer[MC + i + n1*buffer_width] = in[i + bottom_left_pixel];	//moves existing middle pixels to buffer without bottom padding sides

	//pads added line
	for (i = 0; i < MC; ++i){
		buffer[buffer_size - i] = buffer[buffer_size - MC - 1];						//pads right side
		buffer[buffer_size - buffer_width + 1 + i] = buffer[buffer_size - (buffer_width - MC - 1)];					//pads left side
	}
}