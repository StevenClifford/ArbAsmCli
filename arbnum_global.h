#ifndef GLOBAL_DEF
#define GLOBAL_DEF
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define maxKeywordLen 10
#define maxArgumentAmount 4
#define libAmount 2
#define recursionDepth 16
#define initialuserInputLen 256

typedef struct FileInfo {
	FILE* fp;
	int rdpos;
	int lineNr;
	time_t begin_time;
} file_t;

typedef struct __global__ {
	char* userInput;
	int userInputLen;
	int readhead;

	char lookingMode;
	// 'i' is for instruction mnemonics
	// 'a' is for arguments
	// 'd' is for done
	// 'e' is for execute
	int libNr;
	int instructNr;
	int argumentNr;

	bool running;
	int scriptLoops;
	char inputMode;
	// 'i' is from stdin
	// 'f' is from file
	// 'w' is there already was a command
	// if they are capitized there was an error

	int bigEndian;
	
	int fileNr;
	file_t* flist;

	char debug;
	// 's' is silent
	// 'v' is verbose
} GLOBAL;

typedef int (*fun)(GLOBAL*);

int strlook(char string[], char source[][maxKeywordLen], int* readhead){
	int item = 0;
	int j;
	bool isthis;
	while (source[item][0] != '\0'){
		isthis = true;
		j = 0;
		while (source[item][j] != '\0'){
			if(string[j + *readhead] != source[item][j]) isthis = false;
			j++;
		}
		if(isthis){
			*readhead += --j;
			return item;
		}
		item++;
	}
	return -1;
}

#endif
