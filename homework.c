#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <stdbool.h>

#define ON 1
#define OFF 0

int *matrix, *sendCounts, *displs, *recvCounts, *recvDispls, *localBuffer, *buffer, *additonalLines, *startAboveLine, *startBelowLine, *aboveLine, *belowLine;
int numSteps, rows, columns, rank, nProcesses, totalElements, approximateValue, nrON;

MPI_Request request, request1;

void compareMatrixs(char *file)
{
    int *m2;
    FILE *f = fopen(file, "r");

    if (fscanf(f, "%d %d", &rows, &columns) != 2)
    {
        fprintf(stderr, "Error reading dimensions from file\n");
        exit(EXIT_FAILURE);
    }

    m2 = (int *)calloc(rows * columns, sizeof(int));

    for (int i = 0; i < rows * columns; i++)
    {
        if (fscanf(f, "%d", &m2[i]) != 1)
        {
            fprintf(stderr, "Error reading dimensions from file\n");
            exit(EXIT_FAILURE);
        }
    }

    fclose(f);

    for (int i = 0; i < rows * columns; i++)
    {
        if (m2[i] != matrix[i])
        {
            printf("Matricile nu corespund la elem a[%d][%d]\n", i / columns, i % columns);
            return;
        }
    }

    printf("Matricile corespund!\n");
}

void printMatrix()
{
    printf("Matrix is with rows %d and columns %d: \n", rows, columns);
    for (int i = 0; i < rows * columns; i++)
    {
        printf("%d ", matrix[i]);

        if (i % columns == (columns - 1) && i != 0)
            printf("\n");
    }
}

void getArgs(int argc, char **argv)
{
    // if (argc != 4)
    // {
    //     printf("Incorrect arguments!\n");
    //     exit(EXIT_FAILURE);
    // }

    numSteps = atoi(argv[3]);
}

void init(char *file)
{
    FILE *f = fopen(file, "r");

    if (fscanf(f, "%d %d", &rows, &columns) != 2)
    {
        fprintf(stderr, "Error reading dimensions from file\n");
        exit(EXIT_FAILURE);
    }
    matrix = (int *)calloc(rows * columns, sizeof(int));

    fclose(f);

    sendCounts = (int *)calloc(nProcesses, sizeof(int));
    displs = (int *)calloc(nProcesses, sizeof(int));
    startAboveLine = (int *)calloc(nProcesses, sizeof(int));
    startBelowLine = (int *)calloc(nProcesses, sizeof(int));

    approximateValue = (rows / nProcesses + 1) * columns;

    localBuffer = (int *)calloc(approximateValue, sizeof(int));
    buffer = (int *)calloc(approximateValue, sizeof(int));

    additonalLines = (int *)calloc(2 * columns, sizeof(int));
    aboveLine = (int *)calloc(columns, sizeof(int));
    belowLine = (int *)calloc(columns, sizeof(int));
}

void readMatrix(char *file)
{
    FILE *f = fopen(file, "r");

    if (fscanf(f, "%d %d", &rows, &columns) != 2)
    {
        fprintf(stderr, "Error reading dimensions from file\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < rows * columns; i++)
    {
        if (fscanf(f, "%d", &matrix[i]) != 1)
        {
            fprintf(stderr, "Error reading dimensions from file\n");
            exit(EXIT_FAILURE);
        }
    }

    fclose(f);
}

void writeMatrix(char *file)
{
    FILE *f = fopen(file, "w");

    fprintf(f, "%d %d\n", rows, columns);

    // printf("write in '%s': ", file);
    for (int i = 0; i < rows * columns; i++)
    {
        fprintf(f, "%d ", matrix[i]);

        if (i % columns == (columns - 1))
            fprintf(f, "\n");
    }

    fclose(f);
}

void printPerProcess()
{
    printf("\nProcess %d: sendCounts= %d aboveLine= ", rank, sendCounts[rank]);
    for (int i = 0; i < columns; i++)
        printf("%d ", aboveLine[i]);
    printf("\n");

    printf("\nProcess %d: belowLine= ", rank);
    for (int i = 0; i < columns; i++)
        printf("%d ", belowLine[i]);
    printf("\n");

    // for (int i = 0; i < sendCounts[rank]; i++)
    // {
    //     if (i % columns == 0)
    //         printf("Process %d: ", rank);

    //     printf("a[%d][%d]= %d ", i / columns, i % columns, localBuffer[i]);

    //     if (i % columns == (columns - 1))
    //         printf("\n");
    // }
}

void sendAdditionalLines()
{
    //printf("sendAdditionalLines() columns: %d\n", columns);

    int *copy1 = (int *)calloc(columns, sizeof(int));
    int *copy2 = (int *)calloc(columns, sizeof(int));

    for (int i = 0; i < 1; i++)
    {
        // send first and second line
        for (int j = 0; j < columns; j++)
        {
            // if (!(startAboveLine[i] >= 0 || startBelowLine[i] <= (rows * columns - columns)))
            //     break;

            //printf("Column %d: ", j);
            if (startAboveLine[i] >= 0){
                //printf("Has aboveLine");
                copy1[j] = matrix[startAboveLine[i] + j];
            }
            if (startBelowLine[i] <= (rows * columns - columns)){
                //printf("Has belowLine");
                copy2[j] = matrix[startBelowLine[i] + j];
            }
        }
    }

    for (int i = 1; i < nProcesses; i++)
    {
        //printf("For process %d: \n", rank);
        // send first and second line
        //memset(additonalLines, 0, 2 * columns * sizeof(int));
        memset(aboveLine, 0, columns * sizeof(int));
        memset(belowLine, 0, columns * sizeof(int));
        for (int j = 0; j < columns; j++)
        {
            // if (!(startAboveLine[i] >= 0 || startBelowLine[i] <= (rows * columns - columns)))
            //     break;

            //printf("Column %d: ", j);
            if (startAboveLine[i] >= 0){
                //printf("Has aboveLine");
                aboveLine[j] = matrix[startAboveLine[i] + j];
            }
            if (startBelowLine[i] <= (rows * columns - columns)){
                //printf("Has belowLine");
                belowLine[j] = matrix[startBelowLine[i] + j];
            }
    
            //printf("\n");
        }
        
        //printf("Process %d: Send additonalLines to %d\n", rank, i);

        // printf("\nFrom root: Process %d: sendCounts= %d aboveLine= ", i, sendCounts[i]);
        // for (int i = 0; i < columns; i++)
        //     printf("%d ", aboveLine[i]);
        // printf("\n");

        // printf("\nFrom root: Process %d: belowLine= ", i);
        // for (int i = 0; i < columns; i++)
        //     printf("%d ", belowLine[i]);
        // printf("\n");

        //MPI_Send(additonalLines, 2 * columns, MPI_INT, i, 0, MPI_COMM_WORLD);
        MPI_Isend(aboveLine, columns, MPI_INT, i, 0, MPI_COMM_WORLD, &request);
        MPI_Isend(belowLine, columns, MPI_INT, i, 1, MPI_COMM_WORLD, &request1);
        //printf("Process %d: Receive additonalLines\n", rank);
        //printf("Bla %d\n", rank);

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
    //printf("Divide tasks!!\n");

    // divide tasks
    for (int i = 0; i < nProcesses; i++)
    {
        sendCounts[i] = (rows / nProcesses + (i < rows % nProcesses ? 1 : 0)) * columns;
        startAboveLine[i] = totalElements - columns;

        displs[i] = totalElements;
        totalElements += sendCounts[i];

        startBelowLine[i] = totalElements;

        //printf("For %d have startAbove= %d and startBelow= %d\n", i, startAboveLine[i], startBelowLine[i]);

        // printf("For process %d have: startAboveLine= %d and startBelowLine= %d and max is= %d\n", i, startAboveLine, startBelowLine, (rows * columns - columns));
    }
}

void checkAboveLine(int i, int j, int *pNrON)
{
    int nrON = 0;

    if (localBuffer[(i - 1) * columns + j])
    {
        // printf("(ABOVE) ");
        nrON++;
    }

    if (j > 0 && localBuffer[(i - 1) * columns + j - 1])
    {
        // printf("(ABOVE - LEFT) ");
        nrON++;
    }

    if (j < (columns - 1) && localBuffer[(i - 1) * columns + j + 1])
    {
        // printf("(ABOVE - RIGHT) ");
        nrON++;
    }

    (*pNrON) += nrON;
}

void checkBelowLine(int i, int j, int *pNrON)
{
    int nrON = 0;

    if (localBuffer[(i + 1) * columns + j])
    {
        // printf("(BELOW) ");
        nrON++;
    }

    if (j > 0 && localBuffer[(i + 1) * columns + j - 1])
    {
        // printf("(BELOW - LEFT) ");
        nrON++;
    }

    if (j < (columns - 1) && localBuffer[(i + 1) * columns + j + 1])
    {
        // printf("(BELOW - RIGHT) ");
        nrON++;
    }

    (*pNrON) += nrON;
}

int neighboursON(int i, int j)
{
    nrON = 0;
    // printf("Process %d with sendConuts= %d: a[%d][%d]= ", rank, sendCounts[rank], i, j);

    // check the before and after elements
    if (j > 0 && localBuffer[i * columns + j - 1])
    {
        // printf("(BEFORE) ");
        nrON++;
    }

    if (j < (columns - 1) && localBuffer[i * columns + j + 1])
    {
        // printf("(AFTER) ");
        nrON++;
    }

    // if is the first or last row from localBuffer check the additionalLines and half of localBuffer else check the hole localBuffer
    if (i == (sendCounts[rank] / columns - 1))
    {
        if (belowLine[j])
        {
            // printf("(BELOW) ");
            nrON++;
        }

        if (j > 0 && belowLine[j - 1])
        {
            // printf("(BELOW - LEFT) ");
            nrON++;
        }

        if (j < (columns - 1) && belowLine[j + 1])
        {
            // printf("(BELOW - RIGHT) ");
            nrON++;
        }

        if (i != 0)
        {
            checkAboveLine(i, j, &nrON);
        }
        else
        {
            if (aboveLine[j])
            {
                // printf("(ABOVE) ");
                nrON++;
            }

            if (j > 0 && aboveLine[j - 1])
            {
                // printf("(ABOVE - LEFT) ");
                nrON++;
            }

            if (j < (columns - 1) && aboveLine[j + 1])
            {
                // printf("(ABOVE - RIGHT) ");
                nrON++;
            }
        }
    }
    else if (i > 0)
    {

        checkAboveLine(i, j, &nrON);
        checkBelowLine(i, j, &nrON);
    }
    else if (i == 0)
    {
        if (aboveLine[j])
        {
            // printf("(ABOVE) ");
            nrON++;
        }

        if (j > 0 && aboveLine[j - 1])
        {
            // printf("(ABOVE - LEFT) ");
            nrON++;
        }

        if (j < (columns - 1) && aboveLine[j + 1])
        {
            // printf("(ABOVE - RIGHT) ");
            nrON++;
        }

        checkBelowLine(i, j, &nrON);
    }

    // printf("'a[%d][%d]: %d' ", i, j, nrON);

    return nrON;
}

void changeMatrix()
{
    // printf("Process %d: Enter changeMatrix()\n", rank);
    int i = 0, j = 0;

    while (i < (sendCounts[rank] / columns))
    {

        if (localBuffer[i * columns + j] == OFF)
        {
            // change to ON state if has 3 or 6 neighbours ON
            if (neighboursON(i, j) == 3 || neighboursON(i, j) == 6)
                buffer[i * columns + j] = ON;
        }

        else
        {
            // change to OFF if hasn't 2 or 3 neighbours ON
            if (neighboursON(i, j) == 2 || neighboursON(i, j) == 3)
                buffer[i * columns + j] = ON;
            else
                buffer[i * columns + j] = OFF;
        }

        // if (rank == 0 && i < 3)
        //     printf("'a[%d][%d]: %d' ", i, j, nrON);

        j++;
        if (j == columns)
        {
            i++;
            j = 0;
        }
    }
}

void waitAdditionalLines()
{
    // int flag1 = 0, flag2 = 0, countSec = 0;
    // bool first = false, second = false;
    MPI_Status status, status1;
    
    MPI_Irecv(aboveLine, columns, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);
    MPI_Irecv(belowLine, columns, MPI_INT, 0, 1, MPI_COMM_WORLD, &request1);
    
    while (1){

        // MPI_Test(&request, &flag1, MPI_STATUS_IGNORE);
        // MPI_Test(&request1, &flag2, MPI_STATUS_IGNORE);

        // if (!first && flag1){
        //     //printf("%d: Receive aboveLine\n", rank);
        //     first = true;
        // }

        // if (!second && flag2){
        //     //printf("%d: Receive belowLine!\n", rank);
        //     second = true;
        // }

        MPI_Wait(&request, &status);
        MPI_Wait(&request1, &status1);

        if (status.MPI_SOURCE == 0 && status1.MPI_SOURCE == 0)
            break;
    }

    // printf("\nProcess %d: sendCounts= %d aboveLine= ", rank, sendCounts[rank]);
    // for (int i = 0; i < columns; i++)
    //     printf("%d ", aboveLine[i]);
    // printf("\n");

    // printf("\nProcess %d: belowLine= ", rank);
    // for (int i = 0; i < columns; i++)
    //     printf("%d ", belowLine[i]);
    // printf("\n");

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
        //printMatrix();
        divideTasks();
    }

    MPI_Bcast(sendCounts, nProcesses, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs, nProcesses, MPI_INT, 0, MPI_COMM_WORLD);

    //printf("Start iterations!!!\n");
    for (int iter = 0; iter < numSteps; iter++)
    {
        //printf("Process %d with iter= %d\n", rank, iter);
        if (rank == 0){
            sendAdditionalLines();
        }
        else
            waitAdditionalLines();

        //printf("%d: Use Scatterv\n", rank);
        MPI_Scatterv(matrix, sendCounts, displs, MPI_INT, localBuffer, sendCounts[rank], MPI_INT, 0, MPI_COMM_WORLD);

        //if (rank == 0)
        //printPerProcess();
        changeMatrix();

        MPI_Gatherv(buffer, sendCounts[rank], MPI_INT, matrix, sendCounts, displs, MPI_INT, 0, MPI_COMM_WORLD);
    }

    if (rank == 0)
    {
        // printMatrix();
        writeMatrix(argv[2]);
        //compareMatrixs(argv[4]);
    }

    MPI_Finalize();

    return 0;
}