// chessboard_ras.c
// ras = runs and size

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "imageBW.h"         
#include "instrumentation.h" 

int main() {
    // Nomear os contadores que serão usados
    InstrName[0] = "runs";
    InstrName[1] = "memory_bytes";
    InstrCalibrate();  


    FILE* csv_file = fopen("chessboard_results.csv", "w");
    if (csv_file == NULL) {
        perror("Falha ao abrir o arquivo CSV");
        return 1;
    }


    fprintf(csv_file, "Size,SquareEdge,Runs,MemoryBytes\n");

    // Arrays de casos de teste para tamanhos e tamanhos de quadrados
    uint32 sizes[] = {100, 500, 1000, 2000, 5000, 10000};
    uint32 square_edges[] = {1, 10, 25, 50, 100};

    for (size_t s_idx = 0; s_idx < sizeof(sizes)/sizeof(sizes[0]); s_idx++) {
        uint32 size = sizes[s_idx];

        for (size_t se_idx = 0; se_idx < sizeof(square_edges)/sizeof(square_edges[0]); se_idx++) {
            uint32 square_edge = square_edges[se_idx];

            // Garantir que square_edge divide size
            if (size % square_edge != 0)
                continue;

            InstrReset();

            // Criar a imagem
            Image img = ImageCreateChessboard(size, size, square_edge, WHITE);

            // Escrever os resultados no CSV
            fprintf(csv_file, "%u,%u,%lu,%lu\n", size, square_edge,
                    InstrCount[0], InstrCount[1]);

            // Liberar a imagem usando ImageDestroy
            ImageDestroy(&img);
        }
    }

    fclose(csv_file);

    printf("Os resultados foram escritos em 'chessboard_results.csv'.\n");

    return 0;
}