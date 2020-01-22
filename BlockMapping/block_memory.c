#include "block_include.h"

int g_megabyte;
table_t* g_mapping;

void init(const int inclination)
{
	int i = 0;
	flash_t data;
	FILE* fp = fopen("flash_memory.txt", "wb");

	if (fp == NULL)
	{
		printf("파일을 열 수 없습니다.\n");
		return;
	}

	if (inclination <= 0)
	{
		printf("명령 구문이 올바르지 않습니다.\n");
		return;
	}

	//할당할 MB : 섹터 * 블록 * 사용자 의향
	g_megabyte = BLOCK * inclination;
	
	//매핑 테이블 생성
	g_mapping = (table_t*)malloc(sizeof(table_t*)*g_megabyte);

	//매핑 테이블 부여
	for (i = 0; i <= g_megabyte; i++)
	{
		(g_mapping + i)->lbn = i;
		(g_mapping + i)->pbn = i;
	}

	//전부 hex값을 00으로 초기화합니다.
	memset(data.byte, 0x00, sizeof(data.byte));

	//파일의 처음으로 간다.
	fseek(fp, 0, SEEK_SET);

	//데이터를 파일에 쓴다.
	for (i = 0; i <= g_megabyte * SECTOR; i++)
	{
		fwrite(&data, sizeof(data), 1, fp);
	}

	printf("%d megabyte(s) flash memory\n", inclination);

	fclose(fp);
}

void flash_read(int lsn)
{
	FILE* fp = fopen("flash_memory.txt", "rb");

	if (fp == NULL)
	{
		printf("파일을 열 수 없습니다.\n");
		return;
	}

	if (lsn <= -1)
	{
		printf("명령 구문이 올바르지 않습니다.\n");
		return;
	}
	else if (lsn > g_megabyte - 1)
	{
		printf("%d 섹터가 없습니다.\n", lsn);
		return;
	}

	ftl_read(fp, lsn);

	fclose(fp);
}

void flash_write(int lsn, const char* string)
{
	clock_t start = clock();
	clock_t end;
	FILE* fp = fopen("flash_memory.txt", "rb+");

	if (fp == NULL)
	{
		printf("파일을 열 수 없습니다.\n");
		return;
	}

	if (lsn <= -1)
	{
		printf("명령 구문이 올바르지 않습니다.\n");
		return;
	}
	else if (lsn > SECTOR * g_megabyte - 1)
	{
		printf("%d 섹터가 없습니다.\n", lsn);
		return;
	}

	ftl_write(fp, lsn, string);
	end = clock();
	printf("실행 시간 : %f초\n", (float)(end - start) / CLOCKS_PER_SEC);
	fclose(fp);
}

void flash_erase(int lbn)
{
	int i;
	int count = 0;
	flash_t data;
	flash_t empty;
	FILE* fp = fopen("flash_memory.txt", "rb+");

	if (lbn <= -1)
	{
		printf("명령 구문이 올바르지 않습니다.\n");
		return;
	}
	else if (lbn > (g_megabyte - 1) / SECTOR)
	{
		printf("%d 블록이 없습니다.\n", lbn);
		return;
	}

	memset(empty.byte, 0x00, sizeof(data.byte));

	//처음에서 number만큼 블록을 이동한다.
	fseek(fp, SECTOR * lbn * sizeof(data), SEEK_SET);

	for (i = 0; i <= SECTOR - 1; i++)
	{
		fwrite(empty.byte, sizeof(data), 1, fp);
		count++;
	}

	printf("%dth block erase, 지우기 : %d회 \n", lbn, count);

	fclose(fp);
}

void ftl_read(FILE* fp, int lsn)
{
	int i;
	const int sector = lsn % SECTOR;
	const int block = lsn / SECTOR;
	flash_t data;

	//구조체 초기화
	memset(data.byte, 0x00, sizeof(data.byte));

	//처음에서 lsn만큼 섹터를 이동한다.
	fseek(fp, ((g_mapping + block)->pbn*SECTOR + sector) * sizeof(data), SEEK_SET);

	//구조체를 읽어온다.
	fread(&data, sizeof(data), 1, fp);
	printf("오프셋 : %d \n16진수 : ", (g_mapping + block)->pbn*SECTOR);

	for (i = 0; i <= sizeof(data.byte) - 1; i++)
	{
		printf("%2X ", data.byte[i]);
	}

	printf("\n문자열 : ");

	for (i = 0; i <= sizeof(data.byte) - 1; i++)
	{
		printf("%2c ", data.byte[i]);
	}

	printf("\n");
}

void ftl_write(FILE* fp, int lsn, const char* string)
{
	int i, j;
	const int limit = (int)(lsn / SECTOR) + 1; //섹터가 다른 블록으로 넘어까지 않도록 제한 설정
	const int sector = lsn % SECTOR;
	const int block = lsn / SECTOR;
	int indicator = (g_mapping + block)->pbn* SECTOR + sector; //파일포인터 지시자
	int count = 0; //한 블록이 꽉찼는지 검사
	int next_block_lsn = 0; //다음 블록 복사용
	flash_t data;

	//원하는 파일 위치 읽기
	fseek(fp, indicator * sizeof(data), SEEK_SET);
	fread(data.byte, sizeof(data), 1, fp);

	//같은 섹터에 쓰면, 다른 블록으로 복사 작업
	if (data.byte[0] != 0x00)
	{
		//복사용 빈 블록 검사
		for (i = 0; i <= g_megabyte - 1; i++)
		{
			//같은 블록이면 건너 뜀
			if ((g_mapping + block)->pbn == i)
			{
				continue;
			}

			count = 0;

			//빈 섹터 찾기
			for (j = 0; j <= SECTOR - 1; j++)
			{
				fseek(fp, (i*SECTOR + j) * sizeof(data), SEEK_SET);
				fread(data.byte, sizeof(data), 1, fp);

				if (data.byte[0] == 0x00)
				{
					count++;
					continue;
				}
				else
				{
					break;
				}
			}

			//빈 블록을 찾으면, 이 블록을 임시 블록으로 복사한다.
			if (count == SECTOR)
			{
				next_block_lsn = i * SECTOR;

				break;
			}
		}

		count = 0;

		for (indicator = (limit - 1)*SECTOR; indicator <= limit * SECTOR - 1; indicator++)
		{
			//최신 데이터 일경우
			if (indicator == lsn)
			{
				fseek(fp, next_block_lsn * sizeof(data), SEEK_SET);
				fwrite(string, strlen(string), 1, fp);
				next_block_lsn++;
				count++;
				continue;
			}

			//기존 섹터 읽기
			fseek(fp, ((g_mapping + block)->lbn + indicator) * sizeof(data), SEEK_SET);
			fread(data.byte, sizeof(data), 1, fp);

			//빈 섹터면 건너뜀
			if (data.byte[0] == 0x00)
			{
				continue;
			}

			//임시 섹터 쓰기
			fseek(fp, next_block_lsn * sizeof(data), SEEK_SET); //다음 블록으로 순서대로 이동
			fwrite(data.byte, strlen(data.byte), 1, fp); //다음 블록으로 복사
			count++;

			if (next_block_lsn >= (limit + 1) * SECTOR)
			{
				break;
			}
			next_block_lsn++;
		}

		//기존 데이터 지우기
		flash_erase(block);

		indicator = 0;
		//임시 데이터를 기존 데이터 복사
		for (next_block_lsn = i * SECTOR; next_block_lsn <= (i + 1)*SECTOR; next_block_lsn++)
		{
			fseek(fp, next_block_lsn * sizeof(data), SEEK_SET);
			fread(data.byte, sizeof(data), 1, fp);
			fseek(fp, (limit - 1 + indicator) * sizeof(data), SEEK_SET); //다음 블록으로 순서대로 이동
			fwrite(data.byte, strlen(data.byte), 1, fp); //다음 블록으로 복사
			count++;
			indicator++;
		}

		//임시 데이터 지우기
		flash_erase(i);
	}
	else
	{
		//실제로 쓴거 쓰기
		fseek(fp, indicator * sizeof(data), SEEK_SET);
		fwrite(string, strlen(string), 1, fp);
		count++;
		(g_mapping + block)->pbn = block;
	}
	printf("done, 쓰기 : %d회\n", count);
}

void print_table(const int inclination)
{
	int i;

	if (inclination > g_megabyte * BLOCK)
	{
		return;
	}

	printf("| %10s %10s |\n", "lbn", "pbn");

	for (i = inclination * SECTOR; i <= (inclination + 1) * SECTOR - 1; i++)
	{
		printf("| %10d %10d |\n", g_mapping[i].lbn, g_mapping[i].pbn);
	}
}