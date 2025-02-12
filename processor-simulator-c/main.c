#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MEM_SIZE 255

// criação e inicialização das variaveis
typedef struct CPU
{
    uint16_t R[8];
    uint16_t PC;
    uint32_t SP;
    uint16_t IR;
    int C, Ov, Z, S;
} CPU;

CPU cpu = {{0}, 0x0000, 0x82000000, 0x0000, 0, 0, 0, 0};
uint8_t memory[MEM_SIZE]; // fazer com 8
uint8_t memoryData[MEM_SIZE];

uint16_t maxAddress = 0;

// funcao para mostrar os estados dos registradores, do sp e pc, alem das flags
void State()
{
    printf("REGISTRADORES:\n");
    for (int i = 0; i < 8; i++)
    {
        printf("R%d: 0x%04X\n", i, cpu.R[i]);
    }
    printf("PC: 0x%04X SP: 0x%08X\n", cpu.PC, cpu.SP);
    printf("FLAGS:\n");
    printf("C: %d\nOv: %d\nZ: %d\nS: %d\n", cpu.C, cpu.Ov, cpu.Z, cpu.S);
}

// leitura do arquivo;
void ReadFile(const char *nameFile)
{
    char line[20]; // linhas

    // armazena cada endereco e cada instrucao
    uint16_t address, instruction;

    // abre o arquivo no modo leitura
    FILE *file = fopen(nameFile, "r");
    if (!file)
    {
        printf("Error ao abrir o arquivo!");
        exit(1);
    }

    // ira ler cada linha
    while (fgets(line, sizeof(line), file) != NULL)
    {
        // condicao que separa endereco e intrucao, e verifica se foram lidas dois parametros
        if (sscanf(line, "%4hx: 0x%4hx", &address, &instruction) == 2)
        {
            // coloca na memoria o endereco e a instrucao daquele endereco
            memory[address] = instruction & 0xFF; // little endian
            memory[address + 1] = (instruction >> 8) & 0xFF; // little endian
            if (address > maxAddress)
            {
                maxAddress = address;
            }
        }
    }

    fclose(file);
}

// funcao que executa as instrucoes do arquivo
void executeInstru()
{
    while (1)
    {

        cpu.IR = memory[cpu.PC] | (memory[cpu.PC + 1] << 8); // little endian
        uint8_t opcode = (cpu.IR & 0xF000) >> 12;

        // aqui é a instrução HALT
        if (opcode == 0xF)
        {
            break;
        }

        if (cpu.PC > maxAddress)
        {
            break;
        }

        cpu.PC += 2;

        switch (opcode)
        {
        case 0x0: // NOP
            State();
            break;
        case 0x1: // MOV
        {
            uint8_t bit11 = (cpu.IR & 0x0800) >> 11;
            uint8_t Rd = (cpu.IR & 0x0700) >> 8;
            if (bit11)
            {
                uint8_t Im = (cpu.IR & 0x00FF);
                cpu.R[Rd] = Im;
            }
            else
            {
                uint8_t Rm = (cpu.IR & 0x0007);
                cpu.R[Rd] = cpu.R[Rm];
            }
        }
        break;
        case 0x2:
        {
            uint8_t bit11 = (cpu.IR & 0x0800) >> 11;
            uint8_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint8_t Rn = (cpu.IR & 0x001C) >> 2;

            uint16_t address = cpu.R[Rm];

            if (bit11)
            {
                uint16_t Im = ((cpu.IR & 0x0700) >> 3) | (cpu.IR & 0x001F);
                
                memoryData[address] = Im & 0xFF; // little endian
                memoryData[address + 1] = (Im >> 8) & 0xFF; // little endian
            }
            else
            {
                memoryData[address] = cpu.R[Rn] & 0xFF; // little endian
                memoryData[address + 1] = (cpu.R[Rn] >> 8) & 0xFF; // little endian
            }
        }
        break;
        case 0x3:
        {
            uint8_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;

            uint16_t address = cpu.R[Rm];

            cpu.R[Rd] = memoryData[address] | (memoryData[address + 1] << 8);
        }
        default:
            break;
        }
    }
}

int main()
{
    ReadFile("../instructions.txt");

    executeInstru();
    State();

    return 0;
}