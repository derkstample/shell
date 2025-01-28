#ifndef SHELL_H
#define SHELL_H

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>

void printCWD();
void scanInput();
void getTokens();
void executeTokens();
int indexOfToken(char* tok);
void stripRedirs(char** inputFile,char** outputFile);

void printTokens();
int getHistIndex();
void storeHistory();

void proc_exit();
int backgroundProc();

int parseInt(char* str);
void getHistory(int index);

#endif