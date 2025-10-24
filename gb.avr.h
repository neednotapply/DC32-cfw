#ifndef _GB_S_H_
#define _GB_S_H_


#define HRAM_OFST_WINPOS	256



void gbCpu(void);

extern unsigned char hram[];
extern unsigned char gbKeys;
unsigned char gbRead(unsigned short addr);
void gbWrite(unsigned short addr, unsigned char val);



#endif