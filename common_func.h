#ifndef COMMON_FUNC_H 
#define COMMON_FUNC_H

int char_to_decimal(char letter);
int hex_to_decimal_4bit(char seq[4]);
int hex_to_decimal_time(char seq[4]);

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

int hex_to_decimal_4bit(char seq[4]){
	int sum = char_to_decimal(seq[1]);
	sum += (char_to_decimal(seq[0])*16);
	sum += (char_to_decimal(seq[3])*16*16);
	sum += (char_to_decimal(seq[2])*16*16*16);
	if(sum > 32767) sum = sum - 65536; 
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

#endif
