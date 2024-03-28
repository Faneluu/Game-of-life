#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <stdbool.h>

#define ON 1
#define OFF 0

int *matrix, *sendCounts, *displs, *recvCounts, *recvDispls, *localBuffer, *buffer;
int numSteps, rows, columns, rank, nProcesses, totalElements, recvTotalElements, nrON, localRows, oldRows, oldColumns;

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

    if (fscanf(f, "%d %d", &rows, &columns) != 2){
        fprintf(stderr, "Error reading");
        exit(EXIT_FAILURE);
    }

    oldRows = rows;
    oldColumns = columns;

    rows += 2;
    columns += 2;

    matrix = (int *)calloc(rows * columns, sizeof(int));

    sendCounts = (int *)calloc(nProcesses, sizeof(int));
    displs = (int *)calloc(nProcesses, sizeof(int));

    recvCounts = (int *)calloc(nProcesses, sizeof(int));
    recvDispls = (int *)calloc(nProcesses, sizeof(int));

    fclose(f);
}

void initPartTwo()
{
    localRows = sendCounts[rank] / columns;

    localBuffer = (int *)calloc(sendCounts[rank], sizeof(int));
    buffer = (int *)calloc(sendCounts[rank], sizeof(int));
}

void readMatrix(char *file)
{
    FILE *f = fopen(file, "r");

   if (fscanf(f, "%d %d", &rows, &columns) != 2){
        fprintf(stderr, "Error reading");
        exit(EXIT_FAILURE);
    }

    rows += 2;
    columns += 2;

    for (int i = 1; i < rows - 1; i++)
    {
        for (int j = 1; j < columns - 1; j++)
            if (fscanf(f, "%d", &matrix[i * columns + j]) != 1){
                fprintf(stderr, "Error reading");
                exit(EXIT_FAILURE);
            }
    }

    fclose(f);
}

void writeMatrix(char *file)
{
    FILE *f = fopen(file, "w");

    fprintf(f, "%d %d\n", (rows - 2), (columns - 2));

    // printf("%d: Received matrix\n", rank);
    // for (int i = 0; i < rows; i++){
    //     for (int j = 0; j < columns; j++){
    //         printf("%d ", matrix[i * columns + j]);
    //     }
    //     printf("\n");
    // }
    // printf("\n");

    for (int i = 1; i < rows - 1; i++)
    {
        for (int j = 1; j < columns - 1; j++)
            fprintf(f, "%d ", matrix[i * columns + j]);
        fprintf(f, "\n");
    }

    //printf("Finish write!\n");

    fclose(f);
}

void divideTasks()
{
    recvTotalElements = columns;

    // divide tasks
    for (int i = 0; i < nProcesses; i++)
    {
        sendCounts[i] = (oldRows / nProcesses + (i < oldRows % nProcesses ? 1 : 0) + 2) * columns;
        recvCounts[i] = (oldRows / nProcesses + (i < oldRows % nProcesses ? 1 : 0)) * columns;

        displs[i] = totalElements;
        recvDispls[i] = recvTotalElements;

        totalElements += (sendCounts[i] - 2 * columns);
        recvTotalElements += recvCounts[i];

        //printf("Process %d: sendCounts= %d, displs= %d, totalElements= %d, recvCounts= %d, recvDispl= %d, recvTotalElements= %d\n", i, sendCounts[i], displs[i], totalElements, recvCounts[i], recvDispls[i], recvTotalElements);
    }
}

int neighboursON(int i, int j)
{
    nrON = 0;

    nrON += localBuffer[(i - 1) * columns + j] + localBuffer[(i - 1) * columns + j - 1] + localBuffer[(i - 1) * columns + j + 1];
    nrON += localBuffer[i * columns + j - 1] + localBuffer[i * columns + j + 1];
    nrON += localBuffer[(i + 1) * columns + j] + localBuffer[(i + 1) * columns + j - 1] + localBuffer[(i + 1) * columns + j + 1];

    //printf("'a[%d][%d]= %d'\t", i - 1, j - 1, nrON);

    return nrON;
}

void changeMatrix()
{
    int i = 0, j = 0;

    // printf("%d: Received localBuffer\n", rank);
    // for (i = 0; i < localRows; i++){
    //     for (j = 0; j < columns; j++){
    //         printf("%d ", localBuffer[i * columns + j]);
    //     }
    //     printf("\n");
    // }
    // printf("\n");

    // take every element
    for (i = 1; i < localRows - 1; i++){
        for (j = 1; j < columns - 1; j++)
        {
            if (localBuffer[i * columns + j] == OFF)
            {
                // change to ON state if has 3 or 6 neighbours ON
                if (neighboursON(i, j) == 3 || neighboursON(i, j) == 6)
                    buffer[(i - 1) * columns + j] = ON;
            }

            else
            {
                // change to OFF if hasn't 2 or 3 neighbours ON
                if (neighboursON(i, j) == 2 || neighboursON(i, j) == 3)
                    buffer[(i - 1) * columns + j] = ON;
                else
                    buffer[(i - 1) * columns + j] = OFF;
            }
        }
    }

    // printf("%d: buffer to send\n", rank);
    // for (i = 0; i < (localRows - 2); i++){
    //     for (j = 0; j < columns; j++){
    //         printf("%d ", buffer[i * columns + j]);
    //     }
    //     printf("\n");
    // }
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
        divideTasks();
        //printf("After divideTasks()\n");
    }

    MPI_Bcast(sendCounts, nProcesses, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displs, nProcesses, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(recvCounts, nProcesses, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(recvDispls, nProcesses, MPI_INT, 0, MPI_COMM_WORLD);

    //printf("%d: Before init\n", rank);
    initPartTwo();
    //printf("%d: After init\n", rank);

   for (int iter = 0; iter < numSteps; iter++)
    {
        // with padding lines
        MPI_Scatterv(matrix, sendCounts, displs, MPI_INT, localBuffer, sendCounts[rank], MPI_INT, 0, MPI_COMM_WORLD);

        changeMatrix();
        //printf("%d: Wait to Gatherv\n", rank);

        // without padding lines
        MPI_Gatherv(buffer, recvCounts[rank], MPI_INT, matrix, recvCounts, recvDispls, MPI_INT, 0, MPI_COMM_WORLD);
        //printf("%d:After to Gatherv\n", rank);
    }

    if (rank == 0)
    {
        //printf("writeMatrix()\n");
        writeMatrix(argv[2]);
    }

    // free(matrix);
    // free(localBuffer);
    // free(buffer);
    // free(sendCounts);
    // free(recvCounts);
    // free(displs);
    // free(recvDispls);

    //printf("%d: Before finalize\n", rank);
    MPI_Finalize();

    //printf("%d: return\n", rank);
    return 0;
}