#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define i8 int8_t
#define u8 uint8_t
#define i16 int16_t
#define u16 uint16_t

void print_byte(u8 byte);
char *getRegister(u8 byte, u8 wide);
char *getEffectiveAddress(u8 mem);
void decode(u8 buff[], u16 size);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("USAGE: decoder path_to_file\n");
    return -1;
  }

  char *fname = argv[1];
  FILE *fd = fopen(fname, "r");

  if (!fd) {
    printf("ERROR: Failed to open file %s\n", fname);
    return -1;
  }

  u8 instBuff[1024];
  memset(&instBuff[0], 0, sizeof(instBuff));

  long read = fread(&instBuff[0], sizeof(u8), 1024, fd);
  // printf("READ %ld bytes\n", read);

  decode(&instBuff[0], read);

  return 0;
}

void print_byte(u8 byte) {
  for (int i = 7; i >= 0; i--) {
    if (byte & (1 << i)) {
      printf("1");
    } else {
      printf("0");
    }
  }
  printf(" ");
}

char *getRegister(u8 byte, u8 wide) {
  u8 reg = byte & 0b11100000 >> 5;

  static char *registersW[] = {"ax", "cx", "dx", "bx", "sp", "bp", "si", "di"};
  static char *registers[] = {"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"};

  if (reg > 8) {
    return "BAD_REG";
  }

  if (wide) {
    return registersW[reg];
  }
  return registers[reg];
}

char *getEffectiveAddress(u8 mem) {
  char *addr[] = {"bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx"};

  if (mem > 8) {
    return "BAD_ADDRESS";
  }

  return addr[mem];
}

void decode(u8 buff[], u16 size) {
  u16 idx = 0;

  while (idx < size) {
    u8 currByte = buff[idx++];
    if ((currByte & 0b10110000) == 0b10110000) {
      // Immediate to register
      u8 reg = currByte & 0b111;
      u8 wide = currByte & 0b1000;

      char *regN = getRegister(reg, wide);

      if (wide) {
        u8 b2 = buff[idx++], b1 = buff[idx++];
        u16 data = (b1 << 8) + b2;
        printf("mov %s,%u\n", regN, data);
      } else {
        u8 data = buff[idx++];
        printf("mov %s,%u\n", regN, data);
      }
    } else if ((currByte & 0b10001000) == 0b10001000) {
      // REGISTER/MEMORY TO/FROM REGISTER
      u8 dest = currByte & 0b10;
      u8 wide = currByte & 0b1;

      u8 nextByte = buff[idx++];
      u8 mod = nextByte >> 6;

      if (mod == 0b00) { // Memory mode, no displacement unless R/M == 110
        u8 reg = (nextByte & 0b00111000) >> 3;
        u8 mem = (nextByte & 0b00000111);

        char *r = getRegister(reg, wide);

        char *from;
        if (mem == 0b110) {
          from = "???";
        } else {
          from = getEffectiveAddress(mem);
        }

        if (dest) {
          printf("mov %s,[%s]\n", r, from);
        } else {
          printf("mov [%s],%s\n", from, r);
        }

      } else if (mod == 0b01) { // Memory mode, 8bit displacement
        u8 reg = (nextByte & 0b00111000) >> 3;
        u8 mem = (nextByte & 0b00000111);

        char *r = getRegister(reg, wide);
        char *addr = getEffectiveAddress(mem);
        u8 disp = buff[idx++];

        if (dest) {
          if (disp) {
            printf("mov %s,[%s+%hu]\n", r, addr, disp);
          } else {
            printf("mov %s,[%s]\n", r, addr);
          }
        } else {
          if (disp) {
            printf("mov [%s+%hu],%s\n", addr, disp, r);
          } else {
            printf("mov [%s],%s\n", addr, r);
          }
        }
      } else if (mod == 0b10) { // Memory mode, 16bit displacement
        u8 reg = (nextByte & 0b00111000) >> 3;
        u8 mem = (nextByte & 0b00000111);

        char *r = getRegister(reg, wide);
        char *addr = getEffectiveAddress(mem);

        u8 b2 = buff[idx++], b1 = buff[idx++];
        u16 disp = (b1 << 8) + b2;

        if (dest) {
          printf("mov %s,[%s+%hu]\n", r, addr, disp);
        } else {
          printf("mov [%s+%hu],%s\n", addr, disp, r);
        }
      } else if (mod == 0b11) { // Register mode, no displacement
        u8 reg = (nextByte & 0b00111000) >> 3;
        u8 rOrM = (nextByte & 0b00000111);

        char *r1 = getRegister(rOrM, wide);
        char *r2 = getRegister(reg, wide);

        printf("mov %s,%s\n", r1, r2);
      }
    }
  }
}
