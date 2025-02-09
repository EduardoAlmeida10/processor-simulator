#include <stdio.h>
#include <stdint.h>

// criação e inicialização das variaveis
typedef struct CPU{
    uint16_t R[8];
    uint16_t PC;
    uint32_t SP;
    uint16_t IR;
    uint8_t C, Ov, Z, S;
}CPU;

CPU cpu = { {0}, 0x0000, 0x0000, 0x82000000, 0, 0, 0, 0 };

int main(){
    
}