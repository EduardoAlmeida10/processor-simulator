#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define MEM_SIZE 255
#define SP_BASE 0x8200

// criação e inicialização das variaveis
typedef struct CPU
{
    uint16_t R[8];
    uint16_t PC;
    uint32_t SP;
    uint16_t IR;
    int C, Ov, Z, S;
} CPU;

CPU cpu = {{0}, 0x0000, 0x8200, 0x0000, 0, 0, 0, 0};
uint8_t memory[MEM_SIZE];
uint8_t memoryData[MEM_SIZE];
uint8_t memoryAccesed[MEM_SIZE] = {0};
uint8_t pilha[255];
bool pilhaAccesed[MEM_SIZE] = {false};

#define SP_END (cpu.SP - SP_BASE)

uint16_t maxAddress = 0;

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
            memory[address] = instruction & 0xFF;            // little endian
            memory[address + 1] = (instruction >> 8) & 0xFF; // little endian
            if (address > maxAddress)
            {
                maxAddress = address;
            }
        }
    }

    fclose(file);
}

// funcao para mostrar os estados dos registradores, do sp e pc, alem das flags e memoria de dados juntamente com a pilha
void State()
{
    printf("REGISTRADORES:\n");
    for (int i = 0; i < 8; i++)
    {
        printf("R%d: 0x%04X\n", i, cpu.R[i]);
    }
    printf("PC: 0x%04X SP: 0x%04X\n", cpu.PC, cpu.SP);
    printf("FLAGS:\n");
    printf("C: %d\nOv: %d\nZ: %d\nS: %d\n", cpu.C, cpu.Ov, cpu.Z, cpu.S);
    printf("MEMORIA DE DADOS:\n");
    for (int i = 0; i < MEM_SIZE; i += 1)
    {
        if (memoryAccesed[i])
        {
            uint16_t value = memoryData[i] | (memoryData[i + 1] << 8);
            printf("0x%04X: 0x%04X\n", i, value);
        }
    }
    printf("PILHA:\n");
    while (cpu.SP > 0x8100)
    {
        cpu.SP -= 2;

        uint16_t value = pilha[SP_END] | (pilha[SP_END + 1] << 8);
        if (pilhaAccesed[SP_END])
        {
            printf("0x%04X: 0x%04X\n", cpu.SP, value);
        }
    }
}

// funcao que executa as instrucoes do arquivo
void executeInstru()
{
    while (1)
    {

        cpu.IR = memory[cpu.PC] | (memory[cpu.PC + 1] << 8); // little endian
        cpu.PC += 2;
        uint8_t opcode = (cpu.IR & 0xF000) >> 12;

        // instrucao HALT
        if (cpu.IR == 0xFFFF)
        {
            break;
        }

        // instrucao invalida
        if ((cpu.IR & 0xF800) >> 11 == 0 && (cpu.IR & 0x0003) == 0 &&
            (cpu.IR & 0x00FC) >> 2 != 0)
        {
            break;
        }

        // instrucao de NOP
        if (cpu.IR == 0x0000)
        {
            State();
        }

        switch (opcode)
        {
        case 0x1: // MOV checked
        {
            uint16_t bit11 = (cpu.IR & 0x0800) >> 11;
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;

            if (bit11)
            {
                uint8_t Im = (cpu.IR & 0x00FF);
                cpu.R[Rd] = Im;
            }
            else
            {
                uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
                cpu.R[Rd] = cpu.R[Rm];
            }
        }
        break;
        case 0x2: // STORE checked
        {
            uint8_t bit11 = (cpu.IR & 0x0800) >> 11;
            uint8_t Rm = (cpu.IR & 0x00E0) >> 5;

            if (bit11)
            {
                uint8_t Im = ((cpu.IR & 0x0700) >> 3) | (cpu.IR & 0x001F);

                memoryData[cpu.R[Rm]] = Im;
                memoryAccesed[cpu.R[Rm]] = 1;
            }
            else
            {
                uint8_t Rn = (cpu.IR & 0x001C) >> 2;
                memoryData[cpu.R[Rm]] = cpu.R[Rn] & 0xFF;
                memoryData[cpu.R[Rm] + 1] = (cpu.R[Rn] >> 8) & 0xFF;
                memoryAccesed[cpu.R[Rm]] = 1;
            }
        }
        break;
        case 0x3: // LOAD checked
        {
            uint8_t Rd = (cpu.IR & 0x0700) >> 8;
            uint8_t Rm = (cpu.IR & 0x00E0) >> 5;
            cpu.R[Rd] = memoryData[cpu.R[Rm]] | (memoryData[cpu.R[Rm] + 1] << 8);
        }
        break;
        case 0x4: // ADD
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t Rn = (cpu.IR & 0x001C) >> 2;

            cpu.R[Rd] = cpu.R[Rm] + cpu.R[Rn];

            uint32_t result = cpu.R[Rm] + cpu.R[Rn];

            if (result > 0xFFFF)
            {
                cpu.C = 1;
            }
            else
            {
                cpu.C = 0;
            }

            if (((cpu.R[Rm] & 0x8000) == (cpu.R[Rn] & 0x8000)) && ((cpu.R[Rd] & 0x8000) != (cpu.R[Rm] & 0x8000)))
            {
                cpu.Ov = 1;
            }
            else
            {
                cpu.Ov = 0;
            }

            if (cpu.R[Rd] == 0)
            {
                cpu.Z = 1;
            }
            else
            {
                cpu.Z = 0;
            }

            if ((cpu.R[Rd] & 0x8000) != 0)
            {
                cpu.S = 1;
            }
            else
            {
                cpu.S = 0;
            }
        }
        break;
        case 0x5: // SUB
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t Rn = (cpu.IR & 0x001C) >> 2;

            cpu.R[Rd] = cpu.R[Rm] - cpu.R[Rn];

            if (cpu.R[Rm] < cpu.R[Rn])
            {
                cpu.C = 1;
            }
            else
            {
                cpu.C = 0;
            }

            if (((cpu.R[Rm] & 0x8000) == 0 && (cpu.R[Rn] & 0x8000) != 0 && (cpu.R[Rd] & 0x8000) != 0) ||
                ((cpu.R[Rm] & 0x8000) != 0 && (cpu.R[Rn] & 0x8000) == 0 && (cpu.R[Rd] & 0x8000) == 0))
            {
                cpu.Ov = 1;
            }
            else
            {
                cpu.Ov = 0;
            }

            if (cpu.R[Rd] == 0)
            {
                cpu.Z = 1;
            }
            else
            {
                cpu.Z = 0;
            }

            if ((cpu.R[Rd] & 0x8000) != 0)
            {
                cpu.S = 1;
            }
            else
            {
                cpu.S = 0;
            }
        }
        break;
        case 0x6: // MUL
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t Rn = (cpu.IR & 0x001C) >> 2;

            cpu.R[Rd] = cpu.R[Rm] * cpu.R[Rn];

            uint32_t result = cpu.R[Rm] * cpu.R[Rn];

            if (result > 0xFFFF)
            {
                cpu.C = 1;
            }
            else
            {
                cpu.C = 0;
            }

            if (result > 0xFFFF)
            {
                cpu.Ov = 1;
            }
            else
            {
                cpu.Ov = 0;
            }

            if (cpu.R[Rd] == 0)
            {
                cpu.Z = 1;
            }
            else
            {
                cpu.Z = 0;
            }

            if ((cpu.R[Rd] & 0x8000) != 0)
            {
                cpu.S = 1;
            }
            else
            {
                cpu.S = 0;
            }
        }
        break;
        case 0x7: // AND
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t Rn = (cpu.IR & 0x001C) >> 2;

            cpu.R[Rd] = cpu.R[Rm] & cpu.R[Rn];

            if (cpu.R[Rd] == 0)
            {
                cpu.Z = 1;
            }
            else
            {
                cpu.Z = 0;
            }

            if ((cpu.R[Rd] & 0x8000) != 0)
            {
                cpu.S = 1;
            }
            else
            {
                cpu.S = 0;
            }
        }
        break;
        case 0x8: // ORR
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t Rn = (cpu.IR & 0x001C) >> 2;

            cpu.R[Rd] = cpu.R[Rm] | cpu.R[Rn];

            if (cpu.R[Rd] == 0)
            {
                cpu.Z = 1;
            }
            else
            {
                cpu.Z = 0;
            }

            if ((cpu.R[Rd] & 0x8000) != 0)
            {
                cpu.S = 1;
            }
            else
            {
                cpu.S = 0;
            }
        }
        break;
        case 0x9: // NOT
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;

            cpu.R[Rd] = ~cpu.R[Rm];

            if (cpu.R[Rd] == 0)
            {
                cpu.Z = 1;
            }
            else
            {
                cpu.Z = 0;
            }

            if ((cpu.R[Rd] & 0x8000) != 0)
            {
                cpu.S = 1;
            }
            else
            {
                cpu.S = 0;
            }
        }
        break;
        case 0xA: // XOR
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t Rn = (cpu.IR & 0x001C) >> 2;

            cpu.R[Rd] = cpu.R[Rm] ^ cpu.R[Rn];

            if (cpu.R[Rd] == 0)
            {
                cpu.Z = 1;
            }
            else
            {
                cpu.Z = 0;
            }

            if ((cpu.R[Rd] & 0x8000) != 0)
            {
                cpu.S = 1;
            }
            else
            {
                cpu.S = 0;
            }
        }
        break;
        case 0xB:
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t Im = cpu.IR & 0x001F;

            cpu.R[Rd] = cpu.R[Rm] >> Im;
        }
        break;
        case 0xC:
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t Im = cpu.IR & 0x001F;

            cpu.R[Rd] = cpu.R[Rm] << Im;
        }
        break;
        case 0xD:
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t LSB = cpu.R[Rm] & 0x0001;

            cpu.R[Rd] = (cpu.R[Rm] >> 1) | (LSB << 15);
        }
        break;
        case 0xE:
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t MSB = (cpu.R[Rm] & 0x8000) >> 15;

            cpu.R[Rd] = (cpu.R[Rm] << 1) | MSB;
        }
        break;
        default:
            break;
        }

        if ((cpu.IR & 0xF800) == 0x0000 && (cpu.IR & 0x0003) == 0x0003) // CMP
        {
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t Rn = (cpu.IR & 0x001C) >> 2;

            int16_t result = cpu.R[Rm] - cpu.R[Rn];

            cpu.Z = (result == 0) ? 1 : 0;
            cpu.S = (cpu.R[Rm] < cpu.R[Rn]) ? 1 : 0;
        }

        // Instrucoes da pilha/desvio
        if ((cpu.IR & 0xF800) == 0x0000 && (cpu.IR & 0x0003) == 0x0001) // PSH
        {
            uint16_t Rn = (cpu.IR & 0x001C) >> 2;

            cpu.SP -= 2;

            pilha[SP_END] = cpu.R[Rn] & 0x00FF;
            pilha[SP_END + 1] = (cpu.R[Rn] & 0xFF00) >> 8;
            pilhaAccesed[SP_END] = true;
        }

        if ((cpu.IR & 0xF800) == 0x0000 && (cpu.IR & 0x0003) == 0x0002) // POP
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;

            cpu.R[Rd] = pilha[SP_END] | (pilha[SP_END + 1] << 8);

            cpu.SP += 2;
        }

        if ((cpu.IR & 0xF000) == 0x0000 && (cpu.IR & 0x0800) == 0x0800) // desvio
        {
            uint16_t Im = (cpu.IR & 0x07FC) >> 2;

            if (Im & 0x0100)
            {
                Im |= 0xFE00;
            }

            uint8_t type = (cpu.IR & 0x0003);

            if (type == 0x0) // JMP
            {
                cpu.PC += Im;
                if (cpu.PC == maxAddress)
                {
                    break;
                }
            }
            else if (type == 0x1) // JEQ
            {
                if (cpu.Z == 1 && cpu.S == 0)
                {
                    cpu.PC += Im;
                    if (cpu.PC == maxAddress)
                    {
                        break;
                    }
                }
            }
            else if (type == 0x2) // JLT
            {
                if (cpu.Z == 0 && cpu.S == 1)
                {
                    cpu.PC += Im;
                    if (cpu.PC == maxAddress)
                    {
                        break;
                    }
                }
            }
            else if (type == 0x3) // JGT
            {
                if (cpu.Z == 0 && cpu.S == 0)
                {
                    cpu.PC += Im;
                    if (cpu.PC == maxAddress)
                    {
                        break;
                    }
                }
            }
        }

        // Para a execucao quando o pc é maior que o ultimo endereco
        if (cpu.PC > maxAddress)
        {
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