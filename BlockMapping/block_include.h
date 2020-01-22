//수정일 : 2019년 11월 29일
#pragma warning(disable:4996)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define SECTOR 32 //1개의 블록은 32개 섹터로 구성
#define BLOCK 64 //64개블록이 모여서 ?
#define MAX 512 //보통 512바이트로 구성
#define SIZE 32 //크기 조정용 값

enum boolean { FALSE, TRUE, BUFFER = 1 };

typedef struct _flash {
	char byte[MAX / SIZE];
} flash_t;

typedef struct _table {
	int lbn;
	int pbn;
} table_t;

void init(const int inclination);
void flash_read(int lsn);
void flash_erase(int lbn);
void flash_write(int lsn, const char* string);
void ftl_read(FILE* fp, int lsn);
void ftl_write(FILE* fp, int lsn, const char* string);
void print_table(const int inclination);
void help();