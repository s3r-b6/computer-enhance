#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define i8 int8_t
#define u8 uint8_t
#define i16 int16_t
#define u16 uint16_t

void decodeInstruction(u8 currByte, u8 *buff, u16 *idx, u16 size);

void print_byte(u8 byte);
char *getRegister(u8 byte, u8 wide);
char *getEffectiveAddress(u8 mem);

// MODS
void immediateToReg(u8 currByte, u8 *buff, u16 *idx);
void registerModeNoDisp(u8 nextByte, u8 currByte);
void memoryModeNoDisp(u8 nextByte, u8 currByte);
void memoryModeWithDisp(u8 nextByte, u8 currByte, u8 *buff, u16 *idx,
                        bool wideDisp);

#define MOV_IMMEDIATE_TO_REG (currByte & 0b10110000) == 0b10110000
#define MOV_R_OR_M_TO_OR_FROM_REG (currByte & 0b10001000) == 0b10001000

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

  u16 size = 1024;

  u8 instBuff[size];
  memset(&instBuff[0], 0, sizeof(instBuff));

  long read = fread(&instBuff[0], sizeof(u8), size, fd);
  // printf("READ %ld bytes\n", read);

  u16 idx = 0;
  while (idx < 1024) {
    u8 currByte = instBuff[idx++];
    decodeInstruction(currByte, instBuff, &idx, size);
  }

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

// Decodes a single instruction
void decodeInstruction(u8 currByte, u8 *buff, u16 *idx, u16 size) {
  if (MOV_IMMEDIATE_TO_REG) {
    printf("mov ");
    immediateToReg(currByte, buff, idx);
  } else if (MOV_R_OR_M_TO_OR_FROM_REG) {
    printf("mov ");

    u8 nextByte = buff[(*idx)++];
    u8 mod = nextByte >> 6;
    switch (mod) {
    case 0b00:
      memoryModeNoDisp(nextByte, currByte);
      break;
    case 0b01:
      memoryModeWithDisp(nextByte, currByte, buff, idx, false);
      break;
    case 0b10:
      memoryModeWithDisp(nextByte, currByte, buff, idx, true);
      break;
    case 0b11:
      registerModeNoDisp(nextByte, currByte);
      break;
    default:
      printf("Error: Unknown addressing mode\n");
    }
  }
}

void immediateToReg(u8 currByte, u8 *buff, u16 *idx) {
  u8 reg = currByte & 0b111;
  u8 wide = (currByte & 0b1000) >> 3;

  char *regN = getRegister(reg, wide);
  if (wide) {
    u16 data = (buff[(*idx) + 1] << 8) + buff[(*idx)];
    printf("%s,%u\n", regN, data);
    *idx += 2;
  } else {
    u8 data = buff[(*idx)++];
    printf("%s,%u\n", regN, data);
  }
}

void memoryModeNoDisp(u8 nextByte, u8 currByte) {
  u8 dest = (currByte & 0b10) >> 1;
  u8 wide = currByte & 0b1;

  u8 reg = (nextByte & 0b00111000) >> 3;
  u8 mem = (nextByte & 0b00000111);

  char *r = getRegister(reg, wide);
  char *from = (mem == 0b110) ? "???" : getEffectiveAddress(mem);

  if (dest) {
    printf("%s,[%s]\n", r, from);
  } else {
    printf("[%s],%s\n", from, r);
  }
}

void memoryModeWithDisp(u8 nextByte, u8 currByte, u8 *buff, u16 *idx,
                        bool wideDisp) {
  u8 dest = (currByte & 0b10) >> 1;
  u8 wide = currByte & 0b1;

  u8 reg = (nextByte & 0b00111000) >> 3;
  u8 mem = (nextByte & 0b00000111);
  char *r = getRegister(reg, wide);
  char *addr = getEffectiveAddress(mem);

  u16 disp = 0;
  if (!wideDisp) { // 8-bit displacement
    disp = buff[(*idx)++];
  } else { // 16-bit displacement
    disp = (buff[(*idx) + 1] << 8) + buff[(*idx)];
    *idx += 2;
  }

  if (dest) {
    printf("%s,[%s+%hu]\n", r, addr, disp);
  } else {
    printf("[%s+%hu],%s\n", addr, disp, r);
  }
}

void registerModeNoDisp(u8 nextByte, u8 currByte) {
  u8 wide = currByte & 0b1;
  u8 reg = (nextByte & 0b00111000) >> 3;
  u8 rOrM = (nextByte & 0b00000111);

  char *r1 = getRegister(rOrM, wide);
  char *r2 = getRegister(reg, wide);

  printf("%s,%s\n", r1, r2);
}
