#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 100
#define WORD_SIZE 4
#define BUFFER_SIZE 200 // reading input from files(200 characters)

typedef struct
{
    char Memory[MEMORY_SIZE][WORD_SIZE]; 
    char R[WORD_SIZE];                   // Register
    char IR[WORD_SIZE];                  // Instruction Registers-current instruction is stored
    unsigned int IC;                     // Instruction Counter to track the current instruction address
    int C;                               // Toggle Value
    int SI;                              // Interrupt  service
    char buffer[BUFFER_SIZE];            // Buffer  hold data temporarily
    FILE *inputfile;                     // file pointer for input file
    FILE *outputfile;
} OS;


void INIT(OS *os);
void LOAD(OS *os);
void EXECUTE(OS *os);
void MOS(OS *os);
void Start(OS *os);
int OppAdd(OS *os);
void READ(OS *os);
void WRITE(OS *os);
void HALT(OS *os);

void INIT(OS *os)
{
    for (int i = 0; i < MEMORY_SIZE; i++)
        for (int j = 0; j < WORD_SIZE; j++)
            os->Memory[i][j] = ' '; // Clear memory by setting all characters to a space

    memset(os->IR, ' ', WORD_SIZE); // Clear instruction register and register
    memset(os->R, ' ', WORD_SIZE);
    os->C = 0;
    os->IC = 0;
}

void LOAD(OS *os)
{ // Load instructions from the input file into memory
    printf("Reading Data...\n");
    int x = 0; // index for memory loccation

    for (int i = 0; i < MEMORY_SIZE; i++) // Clear memory before loading new instructions
        for (int j = 0; j < WORD_SIZE; j++)
            os->Memory[i][j] = ' ';

    do
    {
        memset(os->buffer, ' ', BUFFER_SIZE);                 // // Clear buffer before reading new data
        fgets(os->buffer, sizeof(os->buffer), os->inputfile); // Read a line from the input file

        if (strncmp(os->buffer, "$AMJ", 4) == 0)
        { // Initialize if $AMJ is found
            INIT(os);
        }
        else if (strncmp(os->buffer, "$DTA", 4) == 0)
        { // Start execution if $DTA is found
            Start(os);
        }
        else if (strncmp(os->buffer, "$END", 4) == 0)
        {          // Handle end of data
            x = 0; // Reset index x to print memory contents
            for (int i = 0; i < MEMORY_SIZE; i++)
            { // Print memory index
                printf("M[%d]\t", i);
                for (int j = 0; j < WORD_SIZE; j++)
                {
                    printf("%c", os->Memory[i][j]); // Print each character in the memory word
                }
                printf("\n");
                if (i % 10 == 9)
                    printf("\n"); // additional space
            }
            getchar();
            continue;
        }
        else
        {
            int k = 0;
            int limit = x + 10; // limit to read up to 10 words
            for (; x < limit; ++x)
            {
                for (int j = 0; j < WORD_SIZE; ++j)
                {
                    os->Memory[x][j] = os->buffer[k++];
                }
                if (os->buffer[k] == ' ' || os->buffer[k] == '\n')
                { // stop if space or newline is encountered
                    break;
                }
            }
        }
    } while (!feof(os->inputfile));
}

// Extracts operand from the instruction register and converts it to an integer index

int OppAdd(OS *os)
{
    int add = (os->IR[2] - '0') * 10 + (os->IR[3] - '0'); // Convert characters at IR[2] and IR[3] to an integer value
    return add;
}

// Start executing instructions from memory starting at IC=0

void Start(OS *os)
{
    os->IC = 0;
    EXECUTE(os);
}

// Execute till halt instruction comes

void EXECUTE(OS *os)
{
    while (1)
    {
        for (int i = 0; i < WORD_SIZE; i++) // Load instruction in IR from IC
            os->IR[i] = os->Memory[os->IC][i];

        os->IC++;

        int loc = OppAdd(os);

        if (os->IR[0] == 'G' && os->IR[1] == 'D')  // Set service interrupt for GD
        { 
            os->SI = 1;
            MOS(os);
        }
        else if (os->IR[0] == 'P' && os->IR[1] == 'D')  // PD
        { 
            os->SI = 2;
            MOS(os);
        }
        else if (os->IR[0] == 'H') // HALT
        {
            os->SI = 3;
            MOS(os);
            break;
        }
        else if (os->IR[0] == 'L' && os->IR[1] == 'R')   // LR
        {
            for (int j = 0; j < WORD_SIZE; j++)
                os->R[j] = os->Memory[loc][j];
            printf("Loaded to Register (R) from Memory[%d]: %.*s\n", loc, WORD_SIZE, os->Memory[loc]);
        }
        else if (os->IR[0] == 'S' && os->IR[1] == 'R')   // SR
        { 
            for (int j = 0; j < WORD_SIZE; j++)
                os->Memory[loc][j] = os->R[j];
            printf("Stored Register (R) to Memory[%d]: %.*s\n", loc, WORD_SIZE, os->R);
        }
        else if (os->IR[0] == 'C' && os->IR[1] == 'R')  // CR
        { 
            os->C = (strncmp(os->R, os->Memory[loc], WORD_SIZE) == 0);
            if (os->C)
            {
                fputs("IS SAME\n", os->outputfile);    
                printf("IS SAME\n");
            }
            else
            {
                fputs("NOT SAME\n", os->outputfile);
                printf("NOT SAME\n");
            }
        }
        else if (os->IR[0] == 'B' && os->IR[1] == 'T')  // BT and set control reg
        { 
            if (os->C) 
            {
                os->IC = loc;
                os->C = 0;     //reset the control reg after branching 
            }
        }
    }
}

// check whhich service interrupt is set
void MOS(OS *os)
{
    switch (os->SI)
    {
    case 1:                     //GD
        memset(os->buffer, ' ', sizeof(os->buffer));   //memset-clear the buffer
        READ(os);
        break;
    case 2:                        //PD
        WRITE(os);
        break;
    case 3:
        HALT(os);                   //HALT
        break;
    default:
        break;
    }
}


//Read input data from file and write in memory
void READ(OS *os)
{
    fgets(os->buffer, 42, os->inputfile);
    os->IR[3] = '0';
    int loc = OppAdd(os);
    int k = 0;

//read up to 10 words from the buffer into memory
    for (int l = 0; l < 10; ++l)
    {
        for (int j = 0; j < WORD_SIZE; ++j)
        {
            os->Memory[loc][j] = os->buffer[k++];
        }
        if (strncmp(&os->buffer[k], "$END", 4) == 0)
            return;
        loc++;
    }
}

// Write data from memory to output file
void WRITE(OS *os)
{
    int loc = OppAdd(os);
    printf("Writing to output from memory location: %d\n", loc);

    for (int l = 0; l < 10; ++l)
    {
        for (int j = 0; j < WORD_SIZE; ++j)
        {
            fputc(os->Memory[loc][j], os->outputfile);  //write to output file
            printf("%c", os->Memory[loc][j]); // Print to console 
        }
        loc++;
    }
    fputc('\n', os->outputfile);
}

void HALT(OS *os)
{
    fputs("\n\n", os->outputfile);
}

int main()
{
    OS os;
    printf("Phase 1 Implemented\n\n");
    printf("Press any key to continue...\n");
    getchar();

    os.inputfile = fopen("input.txt", "r");   // input.txt file
    os.outputfile = fopen("output.txt", "w"); // output.txt file

    if (!os.inputfile)
    {
        printf("Input file doesn't exist\n");
        return 1;
    }
    else
    {
        printf("Input file exists\n");
    }

    LOAD(&os);
    fclose(os.inputfile);
    fclose(os.outputfile);
    return 0;
}
