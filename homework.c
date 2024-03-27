#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <stdbool.h>

#define ON 1
#define OFF 0

int *matrix, *sendCounts, *displs, *recvCounts, *recvDispls, *localBuffer, *buffer, *startAboveLine, *startBelowLine, *aboveLine, *belowLine, *realLocalBuffer;
int numSteps, rows, columns, rank, nProcesses, totalElements, approximateValue, nrON,localRows, newRows, newColumns;

    

MPI_Request request, request1;

void getArgs(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Incorrect arguments!\n");
        exit(EXIT_FAILURE);
    }

    numSteps = atoi(argv[3]);
}

void init(char *file)
{
    FILE *f = fopen(file, "r");

    fscanf(f, "%d %d", &rows, &columns);
    matrix = (int *)calloc(rows * columns, sizeof(int));

    fclose(f);

    sendCounts = (int *)calloc(nProcesses, sizeof(int));
    displs = (int *)calloc(nProcesses, sizeof(int));
    startAboveLine = (int *)calloc(nProcesses, sizeof(int));
    startBelowLine = (int *)calloc(nProcesses, sizeof(int));

    approximateValue = (rows / nProcesses + 1) * columns;

    localBuffer = (int *)calloc(approximateValue, sizeof(int));
    buffer = (int *)calloc(approximateValue, sizeof(int));

    aboveLine = (int *)calloc(columns, sizeof(int));
    belowLine = (int *)calloc(columns, sizeof(int));
}

void initPartTwo()
{
    localRows = sendCounts[rank] / columns;
    newRows = localRows + 2;
    newColumns = columns + 2;

    realLocalBuffer = (int *)calloc((newRows * newColumns), sizeof(int));
}

void readMatrix(char *file)
{
    FILE *f = fopen(file, "r");

    fscanf(f, "%d %d", &rows, &columns);

    for (int i = 0; i < rows * columns; i++)
    {
        fscanf(f, "%d", &matrix[i]);
    }

    fclose(f);
}

void writeMatrix(char *file)
{
    FILE *f = fopen(file, "w");

    fprintf(f, "%d %d\n", rows, columns);

    for (int i = 0; i < rows * columns; i++)
    {
        fprintf(f, "%d ", matrix[i]);

        if (i % columns == (columns - 1))
            fprintf(f, "\n");
    }

    fclose(f);
}

void sendAdditionalLines()
{
    int *copy1 = (int *)calloc(columns, sizeof(int));
    int *copy2 = (int *)calloc(columns, sizeof(int));

    for (int i = 0; i < 1; i++)
    {
        // send first and second line
        for (int j = 0; j < columns; j++)
        {
            if (startAboveLine[i] >= 0){
                copy1[j] = matrix[startAboveLine[i] + j];
            }
            if (startBelowLine[i] <= (rows * columns - columns)){
                copy2[j] = matrix[startBelowLine[i] + j];
            }
        }
    }

    for (int i = 1; i < nProcesses; i++)
    {
        // send first and second line
        memset(aboveLine, 0, columns * sizeof(int));
        memset(belowLine, 0, columns * sizeof(int));
        for (int j = 0; j < columns; j++)
        {
            if (startAboveLine[i] >= 0){
                aboveLine[j] = matrix[startAboveLine[i] + j];
            }
            if (startBelowLine[i] <= (rows * columns - columns)){
                belowLine[j] = matrix[startBelowLine[i] + j];
            }
        }

        MPI_Isend(aboveLine, columns, MPI_INT, i, 0, MPI_COMM_WORLD, &request);
        MPI_Isend(belowLine, columns, MPI_INT, i, 1, MPI_COMM_WORLD, &request1);

        MPI_Wait(&request, MPI_STATUS_IGNORE);
        MPI_Wait(&request1, MPI_STATUS_IGNORE);
    }

    for (int i = 0; i < columns; i++){
        aboveLine[i] = copy1[i];
        belowLine[i] = copy2[i];
    }

    free(copy1);
    free(copy2);
}

void divideTasks()
{
    // divide tasks
    for (int i = 0; i < nProcesses; i++)
    {
        sendCounts[i] = (rows / nProcesses + (i < rows % nProcesses ? 1 : 0)) * columns;
        startAboveLine[i] = totalElements - columns;

        displs[i] = totalElements;
        totalElements += sendCounts[i];

        startBelowLine[i] = totalElements;
    }
}

void checkAboveLine(int i, int j)
{
    if (localBuffer[(i - 1) * columns + j])
    {
        nrON++;
    }

    if (j > 0 && localBuffer[(i - 1) * columns + j - 1])
    {
        nrON++;
    }

    if (j < (columns - 1) && localBuffer[(i - 1) * columns + j + 1])
    {
        nrON++;
    }
}

void checkBelowLine(int i, int j)
{
    if (localBuffer[(i + 1) * columns + j])
    {
        nrON++;
    }

    if (j > 0 && localBuffer[(i + 1) * columns + j - 1])
    {
        nrON++;
    }

    if (j < (columns - 1) && localBuffer[(i + 1) * columns + j + 1])
    {
        nrON++;
    }
}

int neighboursON(int i, int j)
{
    nrON = 0;

    nrON += realLocalBuffer[(i - 1) * newColumns + j] + realLocalBuffer[(i - 1) * newColumns + j - 1] + realLocalBuffer[(i - 1) * newColumns + j + 1];
    nrON += realLocalBuffer[i * newColumns + j - 1] + realLocalBuffer[i * newColumns + j + 1];
    nrON += realLocalBuffer[(i + 1) * newColumns + j] + realLocalBuffer[(i + 1) * newColumns + j - 1] + realLocalBuffer[(i + 1) * newColumns + j + 1];

    //printf("'a[%d][%d]= %d'\t", i, j, nrON);

    return nrON;
}

void changeMatrix()
{
    int i = 0, j = 0;

    // for (i = 0; i < rows; i++){
    //     for (j = 0; j < columns; j++){
    //         printf("%d ", localBuffer[i * columns + j]);
    //     }
    //     printf("\n");
    // }
    // printf("\n");

    // add padding
    for (i = 1; i < newRows - 1; i++){
        for (j = 1; j < newColumns - 1; j++){
            realLocalBuffer[i * newColumns + j] = localBuffer[(i - 1) * columns + (j - 1)];
        }
    }

    // add additional lines
    for (j = 1; j < newColumns - 1; j++){
        realLocalBuffer[j] = aboveLine[j - 1];
        realLocalBuffer[(newRows - 1) * newColumns + j] = belowLine[j - 1];
    }

    // for (i = 0; i < newRows; i++){
    //     for (j = 0; j < newColumns; j++){
    //         printf("%d ", realLocalBuffer[i * newColumns + j]);
    //     }
    //     printf("\n");
    // }

    // take every element
    for (i = 1; i < newRows - 1; i++){
        for (j = 1; j < newColumns - 1; j++)
        {
            if (realLocalBuffer[i * newColumns + j] == OFF)
            {
                // change to ON state if has 3 or 6 neighbours ON
                if (neighboursON(i, j) == 3 || neighboursON(i, j) == 6)
                    buffer[(i - 1) * columns + (j - 1)] = ON;
            }

            else
            {
                // change to OFF if hasn't 2 or 3 neighbours ON
                if (neighboursON(i, j) == 2 || neighboursON(i, j) == 3)
                    buffer[(i - 1) * columns + (j - 1)] = ON;
                else
                    buffer[(i - 1) * columns + (j - 1)] = OFF;
            }
        }
    }
}

void waitAdditionalLines()
{
    int flag1 = 0, flag2 = 0;
    bool first = false, second = false;
    
    MPI_Irecv(aboveLine, columns, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
    MPI_Irecv(belowLine, columns, MPI_INT, 0, 1, MPI_COMM_WORLD, &request1);
    
    while (!first || !second){

        MPI_Test(&request, &flag1, MPI_STATUS_IGNORE);
        MPI_Test(&request1, &flag2, MPI_STATUS_IGNORE);

        if (!first && flag1){
            first = true;
        }

        if (!second && flag2){
            second = true;
        }
    }
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

    getArgs(argc, argv);
    init(argv[1]);

    if (rank == 0)
    {
        readMatrix(argv[1]);
        divideTasks();
    }

    MPI_Bcast(sendCounts, nProcesses, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs, nProcesses, MPI_INT, 0, MPI_COMM_WORLD);

    initPartTwo();

   for (int iter = 0; iter < numSteps; iter++)
    {
        if (rank == 0){
            sendAdditionalLines();
        }
        else
            waitAdditionalLines();

        MPI_Scatterv(matrix, sendCounts, displs, MPI_INT, localBuffer, sendCounts[rank], MPI_INT, 0, MPI_COMM_WORLD);

        changeMatrix();

        MPI_Gatherv(buffer, sendCounts[rank], MPI_INT, matrix, sendCounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
    }

    if (rank == 0)
    {
        writeMatrix(argv[2]);
    }

    MPI_Finalize();

    return 0;
}