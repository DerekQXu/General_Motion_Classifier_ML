//BLE_parser.h
#ifndef BLE_parser
#define BLE_parser

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFF_MAX 256

struct Queue *out_ax;
struct Queue *out_ay;
struct Queue *out_az;
struct Queue *out_gx;
struct Queue *out_gy;
struct Queue *out_gz;
struct Queue *out_mx;
struct Queue *out_my;
struct Queue *out_mz;

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
				(*queue_func)(out_ax, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 2:
				(*queue_func)(out_ay, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 3:
				(*queue_func)(out_az, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 4:
				(*queue_func)(out_gx, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 5:
				(*queue_func)(out_gy, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 6: 
				(*queue_func)(out_gz, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 7:
				(*queue_func)(out_mx, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 8:
				(*queue_func)(out_my, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			case 9: 
				(*queue_func)(out_mz, (float)(hex_to_decimal_4bit(&data[index]))); 
				break;
			default:
				return 0;
		}
		index += 4;
	}
	return 1;
}

void init_parsing() {
	out_ax = createQueue();
	out_ay = createQueue();
	out_az = createQueue();
	out_gx = createQueue();
	out_gy = createQueue();
	out_gz = createQueue();
	out_mx = createQueue();
	out_my = createQueue();
	out_mz = createQueue();
}
void destr_parsing() {
	int ret = 1;
	while (ret != 0){
		deQueue(out_ax);	
		deQueue(out_ay);	
		deQueue(out_az);	
		deQueue(out_gx);	
		deQueue(out_gy);	
		deQueue(out_gz);	
		deQueue(out_mx);	
		deQueue(out_my);	
		ret = deQueue(out_mz);	
	}
}
#endif
