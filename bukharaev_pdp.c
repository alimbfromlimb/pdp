/* Program: files.c
   Build me with
     gcc -o files files.c
*/
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>

/*
#define LO(x) (x & 0xFF)
#define HI(x) (((x>>8) & 0xFF))
*/

#define NO_PARAM 0
#define HAS_XX 1
#define HAS_SS (1<<1)
#define HAS_DD (1<<2)

#define RELEASE 0
#define DEBUG 1
#define FULL_DEBUG 2

int debug_level = DEBUG;

//int dbg_lvl = debug_level;

typedef unsigned int byte;
typedef int word;
typedef word adr;

byte mem[64*1024];

// PLEASE MIND THAT FIRST 16 BYTES ARE NOT AVAILABLE AS RAM

word reg[8];
#define sp reg[6]
#define pc reg[7]

void trace(int dbg_lvl, char * format, ...);
void b_write (adr a, byte x);
byte b_read (adr a);
void w_write (adr a, word x);
word w_read (adr a);
void do_halt();
void do_mov();
void do_add();
void do_unknown();
void load_file();
void mem_dump(adr start, word n);
void run();
void print_reg();
void test_mem();

struct SSDD {
	word val;
	adr a;
} ss, dd;

struct Command {
	word opcode;
	word mask;
	const char * name;
	void (*do_func)();
	byte param;
			
}	command[] = {
	{0010000, 0170000, "mov",		do_mov, 	HAS_SS | HAS_DD},
	{0060000, 0170000, "add",		do_add, 	HAS_SS | HAS_DD},	
	{0000000, 0177777, "halt",		do_halt, 	NO_PARAM},
	{0000000, 0170000, "unknown", 	do_unknown, NO_PARAM}	
};

struct SSDD get_mode (word w) {
	struct SSDD result;
	int n = w & 7;
	int mode = (w>>3) & 7;
	switch(mode) {
		case 0:
				result.a = n;
				result.val = reg[n];
				printf("R%d ", n);
				break;
		case 1:
				result.a = reg[n];
				result.val = w_read(result.a);
				printf("(R%d) ", n);
				break;
		case 2:
				result.a = reg[n];
				result.val = w_read(result.a);
				if (n != 7) {
					printf("(R%d)+ ", n);
				}
				else {
					printf(" #%o ", result.val);
				}
				reg[n]+=2;
				break;
				
		//СПРОСИТЬ ПРО ТРЕТИЙ, ЧЕТВЁРТЫЙ И ПЯТЫЙ КЕЙСЫ, ПОЧЕМУ КАРТИНКИ В ПРЕЗЕНТАЦИИ ОДНИ, А КОД ДРУГОЙ?!		
				
		case 3:
				result.a = w_read(reg[n]);
				result.val = w_read(result.a);
				if (n != 7) {
					printf("@(R%d)+ ", n);
				}
				else {
					printf(" #%o ", result.val);
				}

				reg[n]+=2;
				break;
		case 4:
				reg[n] -= 2;
				result.a = reg[n];
				result.val = w_read(result.a);
				if (n != 7) {
					printf("-(R%d) ", n);
				}
				else {
					printf(" #%o ", result.val);
				}
				break; 
		case 5:
				
		
		default:
				printf("Esche ne prohodily 7:( \n");
	}
	return result;
}

int main() {
	load_file();
	//mem_dump(0x40, 4);
	mem_dump(0x200, 0xc);
	run();
	return 0;
}

void trace(int dbg_lvl, char * format, ...) {
	if (dbg_lvl != debug_level)
		return;
	va_list ap;
	va_start (ap, format);
	vprintf(format, ap);
	va_end(ap);	
}
void print_reg() {
	int i;
	for (i = 0; i < 8; ++i) {
		printf("R%d : %.6o\n", i, reg[i]);
	}
}
void b_write (adr a, byte x) {
	mem[a] = x;	
}
byte b_read (adr a) {
	return mem[a];	
}
void w_write (adr a, word x) {
	if (a > 15) {
		assert(!(a % 2));
		mem[a] = (byte)(x & 0xFF);
		mem[a+1] = (byte)((x >> 8) & 0xFF);
	} else {
		reg[a] = x;
	}		
}
word w_read (adr a) {
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
void do_halt() {
	print_reg();
	printf("THE END!\n");
	exit(0);
}
void do_add() {
	//write(dd.a) = ss.val + dd.val;
	w_write(dd.a, dd.val + ss.val);
	return;
}
void do_mov() {
	//write(dd.a) = ss.val;
	w_write(dd.a, ss.val);
	return;
}
void do_unknown() {
	printf("UNKNOWN :(9(((99( I DON'T KNOW WHAT TO DO!!!\n");
	printf("YOU MAY HAVE A LOOK AT MY REGISTERS... IF YOU WILL :)\n");
	print_reg();
	return;
}
void load_file() {
	FILE *f_in = NULL;
	//f_in = fopen("in.txt", "r");
	f_in = stdin;
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
void run() {
	//int count = 0;
	pc = 01000;
	while(1) {
		word w = w_read(pc) & 0xffff;
		fprintf(stdout, "%06o: %06o ", pc, w);
		pc += 2;		
		for(int i = 0; i < 4 ; i++) {
			struct Command cmd = command[i];
			if ((w & cmd.mask) == cmd.opcode) {
				printf("%s ", cmd.name);
				printf(" ");
				if(cmd.param & HAS_SS)
					ss = get_mode(w>>6);
				if(cmd.param & HAS_DD)
					dd = get_mode(w);
				/*
				// dissection of arguments
				if (cmd.param & HAS_SS) {
					//ss = get_mr(w);
				}
				if () {
					
				}
				//~ if (cmd.param & HAS_NN) {
					//~ //nn = get_nn(w);
				//~ }	
				*/			
				printf("\n");
				cmd.do_func();
				print_reg();
				break;
				//count++;
			}
		}
		/*
		if (count == 5)
			break;
		*/
	}	
}
/*
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
	printf("%04 = %02hx%02hx\n", w, b1, b0);
	assert(b0 == 0x0b);
	assert(b1 == 0x0b);
}
*/
void test_mem() {
	b_write(2, 0x0a);
	b_write(3, 0x0b);
	word w = w_read(2);
	printf("0b0a = %hx\n", w);	
	w_write(4, 0x0d0c);
	byte b4 = b_read(4);
	byte b5 = b_read(5);
	printf("0d0c = %hx %hx \n", b5, b4);
}

