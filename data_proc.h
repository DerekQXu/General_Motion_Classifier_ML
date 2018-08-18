//BLE_parser.h
#ifndef DATA_PROC_H
#define DATA_PROC_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <liquid/liquid.h>
#include "common_func.h"
#include "queue.h"

#define BUFF_SIZE 256

// data stored in these queues
struct Queue *ax;
struct Queue *ay;
struct Queue *az;
struct Queue *gx;
struct Queue *gy;
struct Queue *gz;
struct Queue *mx;
struct Queue *my;
struct Queue *mz;

float complex filter_in;
float complex filter_out;
iirfilt_crcf filter_ax;
iirfilt_crcf filter_ay;
iirfilt_crcf filter_az;

struct filter_options {
	unsigned int order; 					// filter order
	float        fc;    					// cutoff frequency
	float        f0;    					// center frequency
	float        Ap;    					// pass-band ripple
	float        As;    					// stop-band attenuation
	liquid_iirdes_filtertype ftype;  		// filter type
	liquid_iirdes_bandtype   btype; 		// bandtype 
	liquid_iirdes_format     format; 		// coefficient format
};

void init_parsing(int capacity) {
	ax = createQueue(capacity);
	ay = createQueue(capacity);
	az = createQueue(capacity);
	gx = createQueue(capacity);
	gy = createQueue(capacity);
	gz = createQueue(capacity);
	mx = createQueue(capacity);
	my = createQueue(capacity);
	mz = createQueue(capacity);
}

void init_filter(){
	// See Documentation:
	// http://liquidsdr.org/doc/iirdes/
	struct filter_options butter;
	butter.order =   2;       // filter order
	butter.fc    =   0.4;    // cutoff frequency
	butter.f0    =   0.0f;    // center frequency
	butter.Ap    =   3.0f;    // pass-band ripple
	butter.As    =   60.0f;   // stop-band attenuation
	butter.ftype  = LIQUID_IIRDES_BUTTER;
	butter.btype  = LIQUID_IIRDES_LOWPASS;
	butter.format = LIQUID_IIRDES_SOS;
	// Create our filters
	filter_ax = iirfilt_crcf_create_prototype(butter.ftype, butter.btype, butter.format, butter.order, butter.fc, butter.f0, butter.Ap, butter.As);
	filter_ay = iirfilt_crcf_create_prototype(butter.ftype, butter.btype, butter.format, butter.order, butter.fc, butter.f0, butter.Ap, butter.As);
	filter_az = iirfilt_crcf_create_prototype(butter.ftype, butter.btype, butter.format, butter.order, butter.fc, butter.f0, butter.Ap, butter.As);
}

int parse_and_filter(char raw[BUFF_SIZE], void (*queue_func)(struct Queue*, float)){
	// find the proper index
	int i; 
	for( i = 0; i < BUFF_SIZE && raw[i] != ':'; ++i);
	i += 2;
	char *ptr = &raw[i];

	// setup lock iter ptr
	int iter = 0;
	char data[40]; 
	int lock = 0;
	while(iter < 40 && ptr != NULL && *ptr != '\0' && *ptr != '\n'){
		if(lock < 40 && *ptr != ' '){ 
			data[iter] = *ptr;
			++lock;
			++iter;
		}
		++ptr;
	}

	int j, k;
	k = 0;
	for(j = 0; j < 10; ++j){
		switch(j){
			case 0:
				//sensortile.timestamp = hex_to_decimal_time(&data[y]);
				break;
			case 1:
				filter_in = (float)(hex_to_decimal_4bit(&data[k]));
				iirfilt_crcf_execute(filter_ax, filter_in, &filter_out);
				(*queue_func)(ax, filter_out); 
				break;
			case 2:
				filter_in = (float)(hex_to_decimal_4bit(&data[k]));
				iirfilt_crcf_execute(filter_ay, filter_in, &filter_out);
				(*queue_func)(ay, filter_out); 
				break;
			case 3:
				filter_in = (float)(hex_to_decimal_4bit(&data[k]));
				iirfilt_crcf_execute(filter_az, filter_in, &filter_out);
				(*queue_func)(az, filter_out); 
				break;
			case 4:
				(*queue_func)(gx, (float)(hex_to_decimal_4bit(&data[k]))); 
				break;
			case 5:
				(*queue_func)(gy, (float)(hex_to_decimal_4bit(&data[k]))); 
				break;
			case 6: 
				(*queue_func)(gz, (float)(hex_to_decimal_4bit(&data[k]))); 
				break;
			case 7:
				(*queue_func)(mx, (float)(hex_to_decimal_4bit(&data[k]))); 
				break;
			case 8:
				(*queue_func)(my, (float)(hex_to_decimal_4bit(&data[k]))); 
				break;
			case 9: 
				(*queue_func)(mz, (float)(hex_to_decimal_4bit(&data[k]))); 
				break;
			default:
				return 0;
		}
		k += 4;
	}
	return 1;
}


void free_parsing() {
	int ret = 1;
	clear(ax);	
	clear(ay);	
	clear(az);	
	clear(gx);	
	clear(gy);	
	clear(gz);	
	clear(mx);	
	clear(my);	
	clear(mz);	
}

#endif
