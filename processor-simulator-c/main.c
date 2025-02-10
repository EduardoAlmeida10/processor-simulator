#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MEM_SIZE 255

// criação e inicialização das variaveis
typedef struct CPU{
    uint16_t R[8];
    uint16_t PC;
    uint32_t SP;
    uint16_t IR;
    int C, Ov, Z, S;
}CPU;

CPU cpu = { {0}, 0x0000, 0x82000000, 0x0000, 0, 0, 0, 0 };
uint16_t memory[MEM_SIZE];

// leitura do arquivo;
void ReadFile(const char *nameFile){
    char line[20]; // linhas

    // armazena cada endereco e cada instrucao
    uint16_t address, instruction;

    // abre o arquivo no modo leitura
    FILE *file = fopen(nameFile, "r");
    if(!file){
        printf("Error ao abrir o arquivo!");
        exit(1);
    }

    // ira ler cada linha
    while(fgets(line, sizeof(line), file) != NULL) {
        // condicao que separa endereco e intrucao, e verifica se foram lidas dois parametros
        if(sscanf(line, "%4hx: 0x%4hx", &address, &instruction) == 2){
            // coloca na memoria o endereco e a instrucao daquele endereco
            memory[address] = instruction;
        }
    }

    fclose(file);
}

// funcao para mostrar os estados dos registradores, do sp e pc, alem das flags
void State(){
    printf("REGISTRADORES:\n");
    for(int i = 0; i < 8; i++){
        printf("R%d: 0x%04X\n", i, cpu.R[i]);
    }
    printf("PC: 0x%04X SP: 0x%08X\n", cpu.PC, cpu.SP);
    printf("FLAGS:\n");
    printf("C: %d\nOv: %d\nZ: %d\nS: %d\n", cpu.C, cpu.Ov, cpu.Z, cpu.S);
}

int main(){
    ReadFile("../instructions.txt");

    State();

    return 0;
}