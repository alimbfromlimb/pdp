/* Program: files.c
   Build me with
     gcc -o files files.c
*/
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

typedef unsigned int byte;
typedef int word;
typedef word adr;

byte mem[56*1024];

void b_write (adr a, byte x) {
	mem[a] = x;	
}

byte b_read (adr a) {
	return mem[a];	
}

void w_write (adr a, word x) {
	assert(!(a % 2));
	mem[a] = (byte)(x & 0xFF);
	mem[a+1] = (byte)((x >> 8) & 0xFF);
}

word w_read  (adr a)
{
    word res;
    assert (a % 2 == 0);
    res = (word)(mem[a]) | (word)(mem[a+1] << 8);
    return res;        
}
/*
void load_file( ) {
	FILE *f_in = NULL;
	f_in = stdin;
	/*
	if (f_in == NULL) {
		perror("in.txt");  // печатаем ошибку открытия файла на чтение, быть может его нет; или файл есть, а у вас нет прав на чтение файла
		return 7;          // даже если тесты проверяющей системой не показаны, код возврата в тесте показан всегда
	}
	
	f_out = fopen("out.txt", "w+");
	if (f_out == NULL) {
		perror("out.txt"); // если файла нет, то его создадут. А если нет прав создать этот файл? Тогда получим диагностику, что не хватает прав на создание
		fclose(f_in);        // хороший тон - закрыть уже открытые нами потоки
		return 8;          // даже если тесты проверяющей системой не показаны, код возврата в тесте показан всегда
	}
	
	int adress, n;
	int i;
	int c = 0;
	while(1) {
		if (2 != fscanf (f_in, "%x%x", &adress, &n))
			return;
		byte x;
		for (i = 1; i < n; ++i) {
			unsigned int x;
			fscanf (f_in, "%x", &x);
			b_write(adress + i, (byte)x);
		}
		
	}
	//fclose (f_in);	
}
*/
void mem_dump(adr start, word n) {
	FILE *f_out = NULL;	
	f_out = stdout;
	int i;
	for (i = 0; i < n; i += 2) {
		word res = w_read(start + i);		
		fprintf (f_out, "%06o : ", start + i);
		fprintf (f_out, "%06o\n", res);	
	}	
}

void load_file( ) {
	FILE *f_in = NULL;
	f_in = fopen("in.txt", "r");
	if (f_in == NULL) {
		perror("in.txt");  // печатаем ошибку открытия файла на чтение, быть может его нет; или файл есть, а у вас нет прав на чтение файла
		exit(1);          // даже если тесты проверяющей системой не показаны, код возврата в тесте показан всегда
	}
	unsigned int adress, n;
	int i;
	//int c = 0;
	while(1) {
		if (2 != fscanf (f_in, "%x%x", &adress, &n))
			return;
		for (i = 0; i < n; ++i) {
			unsigned int x;
			fscanf (f_in, "%x", &x);
			b_write(adress + i, (byte)x);
		}	
	}
	fclose (f_in);	
}

int main() {
	load_file();
	mem_dump(0x40, 4);
	return 0;
}
