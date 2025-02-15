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
uint8_t memory[MEM_SIZE];
uint8_t memoryData[MEM_SIZE];
uint8_t memoryAccesed[MEM_SIZE] = {0};
uint8_t pilha[MEM_SIZE];

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

// funcao que executa as instrucoes do arquivo
void executeInstru()
{
    while (1)
    {

        cpu.IR = memory[cpu.PC] | (memory[cpu.PC + 1] << 8); // little endian
        cpu.PC += 2;
        uint8_t opcode = (cpu.IR & 0xF000) >> 12;

        // aqui é a instrução HALT
        if (opcode == 0xF)
        {
            break;
        }

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
            uint16_t bit11 = (cpu.IR & 0x0800) >> 11;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;

            if (bit11)
            {
                uint16_t Im = ((cpu.IR & 0x0700) >> 3) | (cpu.IR & 0x001F);

                memoryData[cpu.R[Rm]] = Im;
                memoryAccesed[cpu.R[Rm]] = 1;
            }
            else
            {
                uint16_t Rn = (cpu.IR & 0x001C) >> 2;
                memoryData[cpu.R[Rm]] = cpu.R[Rn] & 0xFF;
                memoryData[cpu.R[Rm] + 1] = (cpu.R[Rn] >> 8) & 0xFF;
                memoryAccesed[cpu.R[Rm]] = 1;
            }
        }
        break;
        case 0x3: // LOAD checked
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;

            cpu.R[Rd] = memoryData[cpu.R[Rm]] | (memoryData[cpu.R[Rm] + 1] << 8);
        }
        break;
        case 0x4: // ADD
        {
            uint16_t Rd = (cpu.IR & 0x0700) >> 8;
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t Rn = (cpu.IR & 0x001C) >> 2;

            cpu.R[Rd] = cpu.R[Rm] + cpu.R[Rn];

            if (cpu.R[Rd] > 0xFFFF)
            {
                cpu.C = 1;
            }
            else
            {
                cpu.C = 0;
            }

            // Overflow

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

            if (cpu.R[Rd] > 0xFFFF)
            {
                cpu.C = 1;
            }
            else
            {
                cpu.C = 0;
            }

            // Overflow

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

            if (cpu.R[Rd] > 0xFFFF)
            {
                cpu.C = 1;
            }
            else
            {
                cpu.C = 0;
            }

            // Overflow

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

        // if ((cpu.IR & 0xF800) == 0x0000 && (cpu.IR & 0x0003) == 0x0001) // PSH
        // {
        //     uint16_t Rn = (cpu.IR & 0x001C) >> 2;

        //     cpu.SP -= 2;
        //     pilha[cpu.SP] = cpu.R[Rn] & 0xFF;
        //     pilha[cpu.SP + 1] = (cpu.R[Rn] & 0xFF) >> 8;
        // }

        // if ((cpu.IR & 0xF800) == 0x0000 && (cpu.IR & 0x0003) == 0x0002) // POP
        // {
        //     uint16_t Rd = (cpu.IR & 0x0700) >> 8;

        //     cpu.R[Rd] = pilha[cpu.SP] | (pilha[cpu.SP + 1] << 8);

        //     cpu.SP += 2;
        // }

        if ((cpu.IR & 0xF800) == 0x0000 && (cpu.IR & 0x0003) == 0x0003) // CMP
        {
            uint16_t Rm = (cpu.IR & 0x00E0) >> 5;
            uint16_t Rn = (cpu.IR & 0x001C) >> 2;

            int16_t result = cpu.R[Rm] - cpu.R[Rn];

            cpu.Z = (result == 0) ? 1 : 0;
            cpu.C = (cpu.R[Rm] < cpu.R[Rn]) ? 1 : 0;
        }

        if (cpu.PC > maxAddress)
        {
            break;
        }
    }
}

// funcao para mostrar os estados dos registradores, do sp e pc, alem das flags e memoria de dados juntamente com a pilha
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
    printf("MEMORIA DE DADOS:\n");
    for (int i = 0; i < MEM_SIZE; i++)
    {
        if (memoryAccesed[i])
        {
            printf("0x%04X: 0x%04X\n", i, memoryData[i]);
        }
    }
    printf("PILHA:\n"); // fazer
}

int main()
{
    ReadFile("../instructions.txt");

    executeInstru();
    State();

    return 0;
}