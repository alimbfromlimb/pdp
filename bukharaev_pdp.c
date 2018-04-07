/* Program: files.c
   Build me with
     gcc -o files files.c
*/
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

#define RELEASE 0
#define DEBUG 1
#define FULL_DEBUG 2

int debug_level = DEBUG;

//int dbg_lvl = debug_level;

void trace(int dbg_lvl, char * format, ...) {
	if (dbg_lvl != debug_level)
		return;
	va_list ap;
	va_start (ap, format);
	vprintf(format, ap);
	va_end(ap);	
}

typedef unsigned int byte;
typedef int word;
typedef word adr;

byte mem[56*1024];

word reg[8];
#define pc reg[7];

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

void mem_dump(adr start, word n) {
	int i;
	for (i = 0; i < n; i += 2) {
		word res = w_read(start + i);		
		trace(debug_level, "%06o : %06o\n", start + i, res);
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

void test_mem() {
	byte b0, b1;
	word w;
	w = 0x0b0a;
	w_write(2, w);
	printf("mem[2] = %x\n", mem[2]);
	printf("HI(0x0b0a) = %x\n", HI(0x0b0a));
	b0 = b_read(2);
	b1 = b_read(3);
	printf("%x %x\n", b1, b0);
	printf("%04 = %02hhx%02hhx\n", w, b1, b0);
	assert(b0 == 0x0b);
	assert(b1 == 0x0b);
}

int main() {
	load_file();
	mem_dump(0x40, 4);
	return 0;
}
