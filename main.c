#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int rows, columns, val;

    rows = atoi(argv[1]);
    columns = atoi(argv[2]);

    FILE *f = fopen(argv[3], "w");

    //srand(time(NULL));

    fprintf(f, "%d %d\n", rows, columns);
    for (int i = 0; i < rows; i++){
        for (int j = 0; j < columns; j++){
            val = rand() % 2;
            fprintf(f, "%d ", val);
        }
        fprintf(f, "\n");
    }

    fclose(f);

    return 0;
}