#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//Universal hashing algorithms implementation. Look at wikipedia for further details.




//A prime number bigger than numBuckets (assuming that number of buckets has a limit) .
int prime = 7103;
int a = 455;
int b = 229;

int universal_hash_int(int value,int numBuckets) {

	return ((a*value+b) % prime ) % numBuckets;
}
int universal_hash_string(char *value,int numBuckets) {

	int h=0;
	for (int i = 0; i < strlen(value); i++ ) {
		h = ((h*a) + value[i]) % numBuckets;
	}

	return h ;
}	
