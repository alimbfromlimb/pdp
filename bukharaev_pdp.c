#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
//These macroses will ILLUSTRATE parameters a word has
#define NO_PARAM 0
#define HAS_XX 1		//Has this strange thing called XX
#define HAS_SS (1<<1) 	//Has source
#define HAS_DD (1<<2) 	//Has destination
#define HAS_NN (1<<3) 	//Has this strange thing called NN
#define HAS_R4 (1<<4) 	//The register byte is on the fourth place: ***R**
#define HAS_R6 (1<<5) 	//The register byte is on the sixth  place: *****R
//I don't actually understand why we need these but...
//...the teacher made us implement them 
#define RELEASE 0
#define DEBUG 1
#define FULL_DEBUG 2

int debug_level = DEBUG;

typedef unsigned int byte;
typedef int word;
typedef word adr;

int u;
//НУЖНО СДЕЛАТЬ ДОСТУПНЫМИ ПЕРВЫЕ БАЙТЫ ПАМЯТИ!!!
byte mem[64*1024];
// PLEASE MIND THAT THE FIRST 16 BYTES ARE NOT AVAILABLE AS RAM
// THEY ARE "REGISTERS"
word reg[8];
#define sp reg[6]
#define pc reg[7]

//tracing
void trace(int dbg_lvl, char * format, ...);
//basic functions
void b_write (adr a, byte x);
byte b_read (adr a);
void w_write (adr a, word x);
word w_read (adr a);
//emulated functions
void do_halt();
void do_mov();
void do_movb();
void do_add();
void do_sob();
void do_clr();
void do_jmp();
void do_br();
void do_beq();
void do_rts();
void do_jsr();
void do_tstb();
void do_bpl();
void do_unknown();
//other functions
void load_file();
void mem_dump(adr start, word n);
void run();
void print_reg();
void NZVC(word x);
void test_mem();
//this will help us write functions
struct SSDD {
	word val;
	adr a;
} ss, dd;

int R4, R6, nn, xx;
//The only reason we need N, Z, V, C is to simplify the implementation
//of some crazy functions
short int N, Z, V, C, BYTE;

#define odata 0177566
#define ostat 0177564
#define ocsr  0177564

struct Command {
	word opcode;
	word mask;
	short int byte_or_not;
	const char * name;
	void (*do_func)();
	byte param;
			
}	command[] = {
	{0010000, 0170000, 	0, "mov",		do_mov, 	HAS_SS | HAS_DD},
	{0110000, 0170000, 	1, "movb",		do_movb, 	HAS_SS | HAS_DD},	
	{0060000, 0170000, 	0, "add",		do_add, 	HAS_SS | HAS_DD},	
	{0000000, 0177777, 	0, "halt",		do_halt, 	NO_PARAM},
	{0077000, 0177000, 	0, "sob",		do_sob, 	HAS_R4 | HAS_NN},
	{0005000, 0177700, 	0, "clr",		do_clr, 	HAS_DD}, 
	{0000100, 0177700, 	0, "jmp",		do_jmp, 	HAS_DD},
	{0000400, 0177400,	0, "br",		do_br, 		HAS_XX},
	{0001400, 0177400,	0, "beq",		do_beq,		HAS_XX},		
	{0000200, 0177770, 	0, "rts",      	do_rts,     HAS_R6},
	{0004000, 0177000, 	0, "jsr",       do_jsr,     HAS_R4 | HAS_DD},
	{0105700, 0177700, 	1, "tstb",     	do_tstb,    HAS_DD},
	{0100000, 0177400, 	1, "bpl",  		do_bpl,     HAS_XX},
	{0000000, 0170000, 	0, "UNKNOWN", 	do_unknown, NO_PARAM}
	//jsr, movb, beq, tstb, bpl, rts	
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
				if (BYTE) {
					result.val = b_read(result.a);
				} else {
					result.val = w_read(result.a);
				}
				printf("(R%d) ", n);
				
				break;
		case 2:
				result.a = reg[n];
				if ((BYTE)&(n!=6)&(n!=7)) {
					result.val = b_read(result.a);
				} else {
					result.val = w_read(result.a);
				}
				if (n != 7) {
					printf("(R%d)+ ", n);
				}
				else {
					printf(" #%o ", result.val);
				}
				reg[n] = ((BYTE)&&(n!=6)&&(n!=7)) ? (reg[n] + 1) : (reg[n] + 2);
				break;		
		case 3: 	
				result.a = w_read(reg[n]);
				if (BYTE) {
					result.val = b_read(result.a);
				} else {
					result.val = w_read(result.a);
				}
				if (n != 7) {
					printf("@(R%d)+ ", n);
				}
				else {
					printf(" @#%o ", result.val);
				}
				reg[n] += 2;			
				break;
		case 4:
				reg[n] = ((BYTE)&&(n!=6)&&(n!=7)) ? (reg[n] - 1) : (reg[n] - 2);
				result.a = reg[n];
				if (BYTE) {
					result.val = b_read(result.a);
				} else {
					result.val = w_read(result.a);
				}
				if (n != 7) {
					printf("-(R%d) ", n);
				}
				else {
					printf(" #%o ", result.val);
				}
				break;
		case 5:
				reg[n] -= 2;
				result.a = w_read(reg[n]);
				if (BYTE) {
					result.val = b_read(result.a);
				} else {
					result.val = w_read(result.a);
				}
				if (n != 7) {
					printf("@-(R%d) ", n);
				}
				else {
					printf(" #%o ", result.val);
				}
				break;
		case 6:
				nn = w_read(pc);
				pc += 2;
				result.a = (reg[n] + nn) & 0177777;
				if (BYTE) {
					result.val = b_read(result.a);
				} else {
					result.val = w_read(result.a);
				}
				if (n != 7) {
					printf("%.6o(R%o) ", nn, n);
				}
				else {
					printf(" #%o ", result.val);
				}
				break;
		case 7:
				nn = w_read(pc);
				pc += 2;
				result.a = w_read(reg[n]);
				result.a = w_read((result.a + nn) & 0177777);
				if (BYTE) {
					result.val = b_read(result.a);
				} else {
					result.val = w_read(result.a);
				}
				if (n != 7) {
					printf("@%.6o(R%o) ", nn, n);
				}
				else {
					printf(" #%o ", result.val);
				}
				break;		 				
		default:
				printf("Esche ne prohodily 7:( \n");
	}
	return result;
}

///////////////////////////////////////////////////
///////////////////////////////////////////////////

int main() {
	load_file();
	//mem_dump(0x40, 4);
	mem_dump(0x200, 0xc);
	w_write(ocsr, 0xFFFF);
	run();
	return 0;
}

///////////////////////////////////////////////////
///////////////////////////////////////////////////

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
	for (i = 0; i < 4; ++i) {
		printf("R%d : %.6o ", i, reg[i]);
	}
	printf("\n");
	for (i = 4; i < 8; ++i) {
		printf("R%d : %.6o ", i, reg[i]);
	}
	printf("\n");
}
void b_write (adr a, byte x) {
	if (a > 15) {
		mem[a] = x;
	} else {
		reg[a] = x & 0xFF;
	}			
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
		reg[a] = x & 0xFF;
	}		
}
word w_read (adr a) {
    word res;
    assert (a % 2 == 0);
    res = (word)(mem[a]) | (word)(mem[a+1] << 8);
    return res;        
}
void do_halt() {
	printf("\n");
	print_reg();
	printf("THE END!\n");
	exit(0);
}
void do_add() {
	//write(dd.a) = ss.val + dd.val;
	w_write(dd.a, dd.val + ss.val);
	/*
	printf("dd.val is: %06o\n", dd.val);
	printf("dd.val+ss.val is: %06o\n", dd.val+ss.val);
	printf("\n");
	mem_dump(0x208, 6);
	*/
	NZVC(dd.val + ss.val);
	return;
}
void do_mov() {
	//write(dd.a) = ss.val;
	w_write(dd.a, ss.val);
	/*
	printf("ss.val is: %06o\n", ss.val);
	printf("\n");
	mem_dump(0x208, 6);
	*/
	NZVC(ss.val);
	return;
}
void do_movb() {
	//write(dd.a) = ss.val;
	b_write(dd.a, ss.val);
	NZVC(ss.val);
	return;
}
void do_sob() {
    reg[R4]--;
    if (reg[R4] != 0)
        pc = pc - (2 * nn);
    printf("R%d\n", R4);
	NZVC(pc);
}
void do_clr() {
	w_write(dd.a, 0);
	NZVC(0);
}
void do_jmp() {
	pc = dd.a;
}
void do_br() {
	pc = pc + (2 * xx);	
}
void do_beq() {
	if (Z == 1)
		do_br();
	printf("%06o ", pc);
}
void do_jsr() {
	/*
	r = pc
	pc = d
	*/
	sp = sp + 2; //push
	w_write(sp, reg[R4]);
	reg[R4] = pc;
	pc = dd.a;
	printf("pc, %06o",pc);
}
void do_rts() {
	pc = reg[R6];
	reg[R6] = w_read(sp);
	sp = sp - 2; //pull
}
void do_bpl() {
	//printf("N is: %d, dd.val is: %06o\n", N, dd.val);
	/*s
	printf("\n");
	mem_dump(0x208, 6);
	*/
	if (N == 0) {
		do_br();
	}
}
void do_tstb() {
	/*
	printf("dd.val is: %06o\n", dd.val);
	printf("\n");
	mem_dump(0x208, 6);
	*/
	NZVC(dd.val);
	C = 0;
	/*
	N = (xx < 0) ? 1 : 0;
    Z = (xx == 0);
    C = 0;
    */
}
void do_unknown() {
	printf(":(9(((99( I DON'T KNOW WHAT TO DO!!!\n");
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
void mem_dump(adr start, word n) {
	printf("\nMEM DUMP\n");
	int i;
	for (i = 0; i < n; i += 2) {
		word res = w_read(start + i);		
		trace(debug_level, "%06o : %06o\n", start + i, res);
	}	
}
void run() {
	//int count = 0;
	printf("\n");
	printf("RUN!!!\n");
	printf("\n");
	pc = 01000;
	while(1) {
		word w = w_read(pc) & 0xffff;
		fprintf(stdout, "%06o: %06o ", pc, w);
		pc += 2;		
		for(int i = 0; ; i++) {
			struct Command cmd = command[i];
			if ((w & cmd.mask) == cmd.opcode) {
				BYTE = cmd.byte_or_not;
				printf("%s ", cmd.name);
				printf(" ");
				if(cmd.param & HAS_SS)
					ss = get_mode(w>>6);
				if(cmd.param & HAS_DD)
					dd = get_mode(w);
				if(cmd.param & HAS_R4)
					R4 = (w >> 6)&7;
				if(cmd.param & HAS_R6)
					R6 = (w		)&7;
				if(cmd.param & HAS_NN)
					nn = w & 63;
				if(cmd.param & HAS_XX)
					xx = (char)(w & 255);				
				cmd.do_func();
				//printf("\n");
				//printf("dd.a is: %06o, dd.val is: %06o.", dd.a, dd.val);
				//мб стоит напечатать dd val после do_func?
				// WHAT'S INSIDE THE REGISTERS AT THE MOMENT???
				//printf("\n");
				//printf(": %06o", b_read(w_read(reg[7])));
				printf("\n");
				printf("-----------------------------------------------\n");
				print_reg();
				printf("-----------------------------------------------\n");
				
				break;
			}
		}
	}	
}
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
void NZVC(word x){
	Z = (x == 0);
	N = (BYTE ? x >> 7 : x >> 15) & 1;
	C = (BYTE ? x >> 8 : x >> 16) & 1;
}
/*
#define LO(x) (x & 0xFF)
#define HI(x) (((x>>8) & 0xFF))

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
* bukharaev_pdp.exe < mode6neg.pdp.o
* bukharaev_pdp.exe < 0arr.txt.o
* bukharaev_pdp.exe < char.pdp.o
*/
