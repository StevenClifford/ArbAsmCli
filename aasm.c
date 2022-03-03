#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "arbnum.h"
#include "arbnum_quats.h"
unsigned int inputlen = 256;
char* userInput;

bool ScriptingMode = false;
int scriptLoops = 0;

int bigEndian = 0;

int stackSize = 20;
num_t* stack;
int stackptr;
num_t* retstack;
int retstackptr;

int regamount = 13;
num_t regs[13];
enum registers {gr1, gr2, gr3, gr4, ir, flag, inplen, endian, stacsz, mstptr, rstptr, tme, loop};
enum instructs {endprog=1, slashn, wincrap, semicolon, h, set, dset, dget, rev, sel,
				inc, dec, add, sub, mul, divi, modu,
				cmp, ucmp, rot, shf, len, 
				print, push, pop, peek, flip, ret,
				SCR, SAVE, LOAD, Ce, Cg, Cs};

const int TheMaximumLengthOfTheThings = 10;
char registerstring[][10] = { "gr1", "gr2", "gr3", "gr4", "ir",
							"flag", "inplen", "endian", "stacsz", "mstptr", "rstptr",
							"time", "loop", "\0end" };
char instructstring[][10] = { "\\", "\n", "\r", ";", "h", "set", "dset", "dget", "rev", "sel",
							"inc", "dec", "add", "sub", "mul", "div", "mod",
							"cmp", "ucmp", "rot", "shf", "len",
							"print", "push", "pop", "peek", "flip", "ret",
							"SCR", "SAVE", "LOAD", "Ce", "Cg", "Cs", "\0end" };

int qregamount = 4;
qua_t qregs[4];
enum quaternionregisters {qr1, qr2, qr3, qr4};
enum quaternioninstructs {qset=1, qnget, qnset, qadd, qsub, qmul, qsmul};

char quatregistring[][10] = { "qr1", "qr2", "qr3", "qr4", "\0end" };
char quatinstructstring[][10] = { "qset", "qnget", "qadd", "qsub", "qmul", "qsmul", "\0end" };

int strlook(char string[], char source[][TheMaximumLengthOfTheThings], int offset, int* lengthoflocated){
	int i = 0;
	int j;
	bool isthis;
	while (source[i][0] != '\0'){
		isthis = true;
		j = 0;
		while (source[i][j] != '\0'){
			if(string[j+offset] != source[i][j]) isthis = false;
			j++;
		}
		*lengthoflocated = j;
		i++;
		if(isthis) return i;
	}
	*lengthoflocated = 1;
	return 0;
}

void functionswitch(int instruction, num_t* args[], bool wasquat, qua_t* qargs[]){
	clock_t begin_time = clock();
	bool doprint = !ScriptingMode;
	num_t* printptr = args[0];
	qua_t* qprintptr = qargs[0];
	num_t dummy;
	initnum(&dummy, 15, 0, 0);
	switch (instruction){
		case set:
			if(wasquat)
				copyquat(qargs[0], qargs[1], 0);
			else
				copynum(args[0], args[1], 0);
			break;
		case dset:
			args[0]->nump[numtoint(args[1], false) % args[0]->len] = args[2]->nump[0];
			break;
		case dget:
			free(dummy.nump);
			initnum(&dummy, 1, args[0]->sign, args[1]->dim);
			dummy.nump[0] = args[0]->nump[numtoint(args[1], false) % args[0]->len];
			copynum(args[0], &dummy, 0);
			break;
		case rev:
			reversenum(args[0]);
			break;
		case sel:
			selectsectionnum(&dummy, args[0], numtoint(args[1], false), numtoint(args[2], false));
			copynum(args[0], &dummy, 0);
			break;
		case inc:
			incnum(args[0], false);
			break;
		case dec:
			incnum(args[0], true);
			break;
		case add:
			if(wasquat)
				sumquat(qargs[0], qargs[0], qargs[1], false);
			else
				sumnum(args[0], args[0], args[1], false);
			break;
		case sub:
			if(wasquat)
				sumquat(qargs[0], qargs[0], qargs[1], true);
			else
				sumnum(args[0], args[0], args[1], true);
			break;
		case mul:
			if(wasquat)
				multquat(qargs[0], qargs[0], qargs[1]);
			else
				multnum(args[0], args[0], args[1]);
			break;
		case divi:
			divnum(args[0], args[2], args[0], args[1]);
			break;
		case modu:
			divnum(args[2], args[0], args[0], args[1]);
			break;
		case cmp:
			inttonum(&regs[flag], cmpnum(args[0], args[1], true));
			printptr = &regs[flag];
			break;
		case ucmp:
			inttonum(&regs[flag], cmpnum(args[0], args[1], false));
			printptr = &regs[flag];
			break;
		case rot:
			rotnum(args[0], numtoint(args[1], true));
			break;
		case shf:
			shiftnum(args[0], numtoint(args[1], true));
			break;
		case print:
			doprint = true;
			break;
		case push:
			stackptr--;
			if(stackptr < 0){
				printf("The bottom of the stack has been reached (it currently contains %d items)\nIt is possible to change the size of the stack by modifying the `stacsz` register, but this will clear the current stack.\n", stackSize);
				stackptr = 0;
				doprint = false;
				break;
			}
			initnum(&stack[stackptr], 1, 0, 0);
			copynum(&stack[stackptr], args[0], 0);
			break;
		case pop:
			if(stackptr >= stackSize){
				printf("The top of the stack has been reached (there are no elements to be popped).\n");
				stackptr = stackSize;
				doprint = false;
				break;
			}
			copynum(args[0], &stack[stackptr], 0);
			free(stack[stackptr].nump);
			stackptr++;
			break;
		case peek:
			if(stackptr == stackSize){
				printf("There are no elements on the stack\n");
				doprint = false;
				break;
			}
			copynum(args[0], &stack[stackptr], 0);
			break;
		case flip:
			if(stackptr >= stackSize){
				printf("The top of the main stack has been reached (there are no elements to be flipped).\n");
				stackptr = stackSize;
				doprint = false;
				break;
			}
			retstackptr--;
			if(retstackptr < 0){
				printf("The bottom of the return stack has been reached (it currently contains %d items)\nIt is possible to change the size of both stacks by modifying the `stacsz` register, but this will clear the current state of both stacks.\n", stackSize);
				retstackptr = 0;
				doprint = false;
				break;
			}
			initnum(&retstack[retstackptr], 1, 0, 0);
			copynum(&retstack[retstackptr], &stack[stackptr], 0);
			printptr = &retstack[retstackptr];
			stackptr++;
			break;
		case ret:
			if(retstackptr >= stackSize){
				printf("The top of the return stack has been reached (there are no elements to be returned).\n");
				retstackptr = stackSize;
				doprint = false;
				break;
			}
			stackptr--;
			if(stackptr < 0){
				printf("The bottom of the main stack has been reached (it currently contains %d items)\nIt is possible to change the size of both stacks by modifying the `stacsz` register, but this will clear the current state of both stacks.\n", stackSize);
				stackptr = 0;
				doprint = false;
				break;
			}
			initnum(&stack[stackptr], 1, 0, 0);
			copynum(&stack[stackptr], &retstack[retstackptr], 0);
			printptr = &stack[stackptr];
			retstackptr++;
			break;
		case len:
			inttonum(args[1], args[0]->len);
			printptr = args[1];
			break;
	}

	if(doprint && !wasquat) printnum(printptr, bigEndian);
	if(doprint && wasquat) printquat(qprintptr, bigEndian);

	inttonum(&regs[tme], (int)((double)(clock() - begin_time)/CLOCKS_PER_SEC));
	free(dummy.nump);
}

void freestack(){
	for(int i = stackptr; i < stackSize; i++)
		free(stack[i].nump);
	free(stack);
	for(int i = retstackptr; i < stackSize; i++)
		free(retstack[i].nump);
	free(retstack);
}

int saveload(char* path[], bool save){
//	int readfilei = 0;
	FILE* fp;
	if(save){
		fp = fopen(*path, "wb+");
		if(fp == NULL){
			printf("Could not open file to save to: '%s'.\n", userInput);
			return 1;
		}
		for(int i = 0; i < regamount; i++)
			savenum(fp, &regs[i]);
		for(int i = stackSize-1; i >= stackptr; i--)
			savenum(fp, &stack[i]);
		for(int i = stackSize-1; i >= retstackptr; i--)
			savenum(fp, &retstack[i]);

		if(!ScriptingMode) printf("Saved state to '%s'.\n", *path);
	} else {
		fp = fopen(*path, "rb");
		if(fp == NULL){
			printf("Could not open file to load from: '%s'.\n", userInput);
			return 1;
		}
		for(int i = 0; i < regamount; i++)
			loadnum(fp, &regs[i]);

		freestack();
		stackSize = numtoint(&regs[stacsz], false);
		stack = (num_t*) malloc(stackSize * sizeof(num_t));
		retstack = (num_t*) malloc(stackSize * sizeof(num_t));
		stackptr = numtoint(&regs[mstptr], false);
		retstackptr = numtoint(&regs[rstptr], false);

		for(int i = stackSize-1; i >= stackptr; i--){
			initnum(&stack[i], 1, 0, 0);
			loadnum(fp, &stack[i]);}
		for(int i = stackSize-1; i >= retstackptr; i--){
			initnum(&retstack[i], 1, 0, 0);
			loadnum(fp, &retstack[i]);}

		if(!ScriptingMode) printf("Loaded state from '%s'.\n", *path);
	}
	fclose(fp);
	return 0;
}

bool dothing(){
	//printf("start dothing\n");
	bool returnbool = true;
	int startat;
	int offsetbegin = 0;
	startofdothing:;
	int instruction = strlook(userInput, instructstring, offsetbegin, &startat);

	int readfilei;//this is used in the things that take strings as arguments

	switch (instruction){
		case 0:
			printf("Not a valid instruction.\n");
			goto donothing;
			break;
		case endprog:
			printf("Good day!\n");
			returnbool = false;
			goto donothing;
			break;
		case slashn:
		case wincrap:
		case semicolon:
			goto donothing;
			break;
		case h:
			printf("To preform an operation, type an instruction mnemonic (e.g. `set`, `print`, `add`, `push`) and add the appropriate amount of arguments seperated by commas.\n\nThe general purpose registers are `gr1`, `gr2`, `gr3` and `gr4`.\n\nTo change notation from little endian (the default) to big endian, set the register `endian` to 1. To change maximum line length, set the register `inplen` to the desired value.\n\nEnter `\\` to close the program.\n\n(P.S. Did you know that the actual plural of \"comma\" is \"commata\"? Wild.)\n");
			goto donothing;
			break;
		case Ce:
			offsetbegin = startat + 1;
			if(regs[flag].nump[0] == 0) goto startofdothing;
			else goto donothing;
			break;
		case Cg:
			offsetbegin = startat + 1;
			if(regs[flag].nump[0] == 1) goto startofdothing;
			else goto donothing;
			break;
		case Cs:
			offsetbegin = startat + 1;
			if(regs[flag].nump[0] == 2) goto startofdothing;
			else goto donothing;
			break;
		case SCR:
			ScriptingMode = true;
			for(readfilei = 4; userInput[readfilei] != '\0' && userInput[readfilei] != '\r' && userInput[readfilei] != '\n'; readfilei++)
				userInput[readfilei - 4] = userInput[readfilei];
			userInput[readfilei - 4] = '\0';

			goto donothing;
			break;
		case SAVE:
			for(readfilei = 5; userInput[readfilei] != '\0' && userInput[readfilei] != '\r' && userInput[readfilei] != '\n'; readfilei++)
				userInput[readfilei - 5] = userInput[readfilei];
			userInput[readfilei - 5] = '\0';

			saveload(&userInput, true);
			goto donothing;
			break;
		case LOAD:
			for(readfilei = 5; userInput[readfilei] != '\0' && userInput[readfilei] != '\r' && userInput[readfilei] != '\n'; readfilei++)
				userInput[readfilei - 5] = userInput[readfilei];
			userInput[readfilei - 5] = '\0';

			saveload(&userInput, false);
			goto donothing;
			break;
	}

	int argam = 3;
	num_t* tmpptr[3];
	num_t tmp[3];

	qua_t* tmpqptr[3];
	qua_t tmpq[3];

	for(int i = 0; i < argam; i++){
		initnum(&tmp[i], 1, 0, 0);
		tmp[i].nump[0] = 0;
		tmpptr[i] = &tmp[i];
		initquat(&tmpq[i]);
		tmpqptr[i] = &tmpq[i];
	}

	int argn = 0;
	int diminq = 0;
	bool wasquat = false;
	int loInputentry = 0;
	char entry;
	bool therewasnosemi = true;
	for(unsigned int i = startat + offsetbegin; i < inputlen; i+=loInputentry){
		loInputentry = 1;
		entry = userInput[i];

		if(entry == ';'){
			therewasnosemi = false;
			break;
		}

		if(argn < argam){
			if(entry == ','){ argn++; diminq = 0;}
			if(entry == '+'){ diminq++; wasquat = true;}
			if( (entry >= 'a' && entry <= 'z') || (entry >= 'A' && entry <= 'Z') ){

				int id;
				if(entry != 'q'){
					id = strlook(userInput, registerstring, i, &loInputentry);
					if(id == 0){
						printf("Register at argument %d is not a register.\n", argn + 1);
						goto donothingsafe;
					} else {
						tmpptr[argn] = &regs[id - 1];
						copynum(&tmpqptr[argn]->q[diminq%4], tmpptr[argn], false);
					}
				} else {
					id = strlook(userInput, quatregistring, i, &loInputentry);
					if(id == 0){
						printf("Register at argument %d is not a (quaternion) register.\n", argn + 1);
						goto donothingsafe;
					} else {
						wasquat = true;
						tmpqptr[argn] = &qregs[id - 1];
						tmpptr[argn] = &qregs[id - 1].q[diminq%4];
//						copynum(tmpptr[argn], &tmpqptr[argn]->q[diminq%4], false);
					}
				}

			} else if (entry >= '0' && entry <= '9'){
			
				loInputentry = inpstrtonum(&tmpqptr[argn]->q[diminq%4], userInput, i, bigEndian);
				tmpptr[argn] = &tmpqptr[argn]->q[diminq%4];
//				copynum(&tmpqptr[argn]->q[diminq%4], tmpptr[argn], false);

			}
		}
	}
	if(therewasnosemi){
		printf("The statement is too long (maximum is %u characters).\nIt is possible to change the maximum by modifying the `inplen` register.\n", inputlen);
		goto donothingsafe;
	}
//	for(int i = 0; i < argam; i++) printquat(tmpqptr[i], bigEndian);

	functionswitch(instruction, tmpptr, wasquat, tmpqptr);

	donothingsafe:
	for(int i; i < argam; i++){
		freequat(&tmpq[i]);
		free(tmp[i].nump);
	}
	donothing:
	return returnbool;
}

void updateessentials(){
	inputlen = numtoint(&regs[inplen], false);
	bigEndian = numtoint(&regs[endian], false);
	scriptLoops = numtoint(&regs[loop], false);
	inttonum(&regs[mstptr], stackptr);
	inttonum(&regs[rstptr], retstackptr);
}

void setessentialsready(){
	inttonum(&regs[inplen], inputlen);
	inttonum(&regs[endian], bigEndian);
	inttonum(&regs[stacsz], stackSize);
	inttonum(&regs[loop], scriptLoops);
	inttonum(&regs[mstptr], stackSize);
	inttonum(&regs[rstptr], stackSize);
}

void flushuserInput(){
	for(unsigned int i = 0; i < inputlen; i++)
		userInput[i] = ';';
}

int main(int argc, char* argv[]){
	initnumarray(regamount, regs, 21, 0, 0);
	initquatarray(qregamount, qregs);
	stackptr = stackSize;
	stack = (num_t*) malloc(stackSize * sizeof(num_t));
	retstackptr = stackSize;
	retstack = (num_t*) malloc(stackSize * sizeof(num_t));

	userInput = (char*) malloc((TheMaximumLengthOfTheThings + inputlen) * sizeof(char));
	flushuserInput();

	FILE* fp = stdin;
	int readheadpos = 0;

	bool running = true;
	bool prevmode = false;
	bool cont = true;
	bool wascommand = false;
	int8_t ataste;
	clock_t begin_time;

	bool using = false;
	int usingarg = 0;
	for(int i = 1; i < argc; i++){
		if(argv[i][0] == '-'){
			switch(argv[i][1]){
				case 'E':
				case 'e':
					cont = false;
					break;
				case 'U':
				case 'u':
					if(i+1 < argc){
						i++;
						saveload(&argv[i], false);
						using = true;
						usingarg = i;
					} else printf("Statefile name missing.\n");
					break;
				case 'B':
				case 'b':
					bigEndian = true;
					break;
			}
		} else {
			sscanf(argv[i], "%s", userInput);
			//printf("userInput: '%s'\n", userInput);
			wascommand = true;
		}
	}

	setessentialsready();

	if(cont) printf("Good to see you!\nEnter `h` for quick tips and `\\` to close the program.\n");

	do {
		if(stackSize != numtoint(&regs[stacsz], false)){
			freestack();
			stackSize = numtoint(&regs[stacsz], false);
			stackptr = stackSize;
			retstackptr = stackSize;
			stack = (num_t*) malloc(stackSize * sizeof(num_t));
			retstack = (num_t*) malloc(stackSize * sizeof(num_t));
		}

		if(ScriptingMode){
			ataste = fgetc(fp);
			fseek(fp, readheadpos, SEEK_SET);
			if(ataste == EOF && scriptLoops == 1){
				readheadpos = 0;
				fseek(fp, readheadpos, SEEK_SET);
			} else if(ataste == EOF){
				inttonum(&regs[tme], (int)((double)(clock() - begin_time)/CLOCKS_PER_SEC));
				printf("Script completed.\n");
				ScriptingMode = false;
				readheadpos = 0;
				fclose(fp);
				fp = stdin;
				wascommand = true;
			}
		}
		
		if(!wascommand){
			if(!ScriptingMode) printf("\\\\\\ ");
			fgets(userInput, inputlen, fp);
//			printf("userInput: '%s', readheadpos: %d\n", userInput, readheadpos);

			for(int i = 0; userInput[i] != '\0' && ScriptingMode; i++)
				readheadpos++;
		} wascommand = false;

		running = dothing();
		updateessentials();

		if(prevmode != ScriptingMode && ScriptingMode){
			fp = fopen(userInput, "r");
			if(fp == NULL){
				printf("Could not open script '%s'.\n", userInput);
				ScriptingMode = false;
				fp = stdin;
			} else begin_time = clock();
			readheadpos = 0;
		} prevmode = ScriptingMode;

		free(userInput);
		userInput = (char*) malloc((TheMaximumLengthOfTheThings + inputlen) * sizeof(char));
		flushuserInput();
//		printf("running %d, cont %d, ScriptingMode %d\n", running, cont, ScriptingMode);
	} while ((running && cont) || ScriptingMode);

	if(using) saveload(&argv[usingarg], true);

	free(userInput);
	freestack();
	freenumarray(regamount, regs);
	freequatarray(qregamount, qregs);
	return 0;
}
