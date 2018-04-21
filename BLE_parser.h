//BLE_parser.h
#ifndef BLE_parser
#define BLE_parser

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <liquid/liquid.h>

#define BUFF_MAX 256
#define QUEUE_MAX 100 // MUST BE LARGER THAN 20 DUE TO GRAVITY

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
	// options
	unsigned int order; 					// filter order
	float        fc;    					// cutoff frequency
	float        f0;    					// center frequency
	float        Ap;    					// pass-band ripple
	float        As;    					// stop-band attenuation
	liquid_iirdes_filtertype ftype;  		// filter type
	liquid_iirdes_bandtype   btype; 		// bandtype 
	liquid_iirdes_format     format; 		// coefficient format
};


int char_to_decimal(char letter){
	switch(letter){
		case '0':
			return 0;
		case '1':
			return 1;
		case '2':
			return 2;
		case '3':
			return 3;
		case '4':
			return 4;
		case '5':
			return 5;
		case '6':
			return 6;
		case '7':
			return 7;
		case '8':
			return 8;
		case '9':
			return 9;
		case 'a':
			return 10;
		case 'b':
			return 11;
		case 'c':
			return 12;
		case 'd':
			return 13;
		case 'e':
			return 14;
		case 'f':
			return 15;
		default:
			return -1;
	}
}

int hex_to_decimal_4bit(char seq[4]){//, int twos_comp){
	int sum = char_to_decimal(seq[1]);
	sum += (char_to_decimal(seq[0])*16);
	sum += (char_to_decimal(seq[3])*16*16);
	sum += (char_to_decimal(seq[2])*16*16*16);

	if(sum > 32767) sum = sum - 65536; //twos_comp == 1 && 

	return sum;
}

int hex_to_decimal_time(char seq[4]){
	int sum2 = char_to_decimal(seq[1]);
	sum2 += (char_to_decimal(seq[0])*16);
	int sum1 = char_to_decimal(seq[3]);
	sum1 += (char_to_decimal(seq[2])*16); 
	sum1 = (sum1 * 1000) + sum2;
	return sum1;
}

int stream_parser(char raw[BUFF_MAX], void (*queue_func)(struct Queue*, float)){
	int i = 0; 
	while(i < BUFF_MAX && raw[i] != ':'){ ++i; }
	i += 2;
	char *ptr = &raw[i];

	int iter = 0;
	char data[40]; 
	int lock = 0;
	while(iter < 40 && ptr != NULL && *ptr != '\0' && *ptr != '\n'){
		if(lock < 40 && *ptr != ' '){ 
			data[iter] = *ptr;
			++lock;
			++iter;
		}
		ptr++;
	}

	int y;
	int index = 0;
	for(y = 0; y < 10; y++){
		switch(y){
			case 0:
				//sensortile.timestamp = hex_to_decimal_time(&data[y]);
				break;
			case 1:
				filter_in = (float)(hex_to_decimal_4bit(&data[index]));
				iirfilt_crcf_execute(filter_ax, filter_in, &filter_out);
				(*queue_func)(ax, filter_out); 
				break;
			case 2:
				filter_in = (float)(hex_to_decimal_4bit(&data[index]));
				iirfilt_crcf_execute(filter_ay, filter_in, &filter_out);
				(*queue_func)(ay, filter_out); 
				break;
			case 3:
				filter_in = (float)(hex_to_decimal_4bit(&data[index]));
				iirfilt_crcf_execute(filter_az, filter_in, &filter_out);
				(*queue_func)(az, filter_out); 
				break;
			case 4:
				(*queue_func)(gx, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 5:
				(*queue_func)(gy, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 6: 
				(*queue_func)(gz, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 7:
				(*queue_func)(mx, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 8:
				(*queue_func)(my, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 9: 
				(*queue_func)(mz, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			default:
				return 0;
		}
		index += 4;
	}
	return 1;
}

void init_parsing() {
	ax = createQueue(QUEUE_MAX);
	ay = createQueue(QUEUE_MAX);
	az = createQueue(QUEUE_MAX);
	gx = createQueue(QUEUE_MAX);
	gy = createQueue(QUEUE_MAX);
	gz = createQueue(QUEUE_MAX);
	mx = createQueue(QUEUE_MAX);
	my = createQueue(QUEUE_MAX);
	mz = createQueue(QUEUE_MAX);

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
void destr_parsing() {
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
