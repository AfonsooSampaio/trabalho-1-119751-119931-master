#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "imageBW.h"
#include "instrumentation.h"

int main(int argc, char* argv[]) {
    uint32_t sizes[] = {10, 20, 50, 100,120, 150, 200}; // Diferentes tamanhos de imagem
    

    // Inicializa os contadores de operações
    ImageInit();

    // Calibrar o tempo de execução
    InstrCalibrate();

    // Iniciar a instrumentação 
    
    double start_time = cpu_time();
    //variável height
    uint32_t height = 150;

    //printf("Pattern,Size,Height,Time(s),Operations\n");
    printf("WIDTH,Time(s),Operations\n");

    // Testa a operação AND para cada padrão separadamente
    // Padrão: Branca com Preta
    for (int i = 0; i < 7; i++) {
        uint32_t size = sizes[i];
        Image white_image = ImageCreate(height, size, WHITE); // Toda branca
        Image black_image = ImageCreate(height, size, BLACK); // Toda preta

    //printf("Branca com Preta");
        InstrReset();
        ImageAND(white_image, black_image);
        printAND(size);
        
        
    }

    /*
    // Padrão: Xadrez com Preta
    for (int i = 0; i < 7; i++) {
        uint32_t size = sizes[i];
        Image image_chess = ImageCreateChessboard(height, size, 1, 0); // Padrão de xadrez
        Image black_image = ImageCreate(height, size, BLACK);          // Toda preta

        //printf("Xadrez com Preta");
        InstrReset();
        ImageAND(image_chess, black_image);
        printAND(size);
        
       
    }

    */
    return 0;
}
