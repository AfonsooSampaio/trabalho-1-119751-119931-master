/// imageBW - A simple image processing module for BW images
///           represented using run-length encoding (RLE)
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// The AED Team <jmadeira@ua.pt, jmr@ua.pt, ...>
/// 2024

// Student authors (fill in below):
// NMec:119751
// Name:Afonso Correia Sampaio
// NMec:119931
// Name:Isabela de Matos Pereira
//
// Date:04/12/2024
//

#include "imageBW.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instrumentation.h"

// The data structure
//
// A BW image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the pointers
// to the RLE compressed image rows.
//
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Constant value --- Use them throughout your code
// const uint8 BLACK = 1;  // Black pixel value, defined on .h
// const uint8 WHITE = 0;  // White pixel value, defined on .h
const int EOR = -1;  // Stored as the last element of a RLE row

// Internal structure for storing RLE BW images
struct image {
  uint32 width;
  uint32 height;
  int** row;  // pointer to an array of pointers referencing the compressed rows
};

// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or
// file (I/O) operations use defensive techniques.
// When one of these functions fails,
// it immediately prints an error and exits the program.
// This fail-fast approach to error handling is simpler for the programmer.

// Use the following function to check a condition
// and exit if it fails.

// Check a condition and if false, print failmsg and exit.
static void check(int condition, const char* failmsg) {
  if (!condition) {
    perror(failmsg);
    exit(errno || 255);
  }
}

/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) {  ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  // Name other counters here...
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
// Add more macros here...

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!

/// Auxiliary (static) functions

/// Create the header of an image data structure
/// And allocate the array of pointers to RLE rows
static Image AllocateImageHeader(uint32 width, uint32 height) {
  assert(width > 0 && height > 0);
  Image newHeader = malloc(sizeof(struct image));
  check(newHeader != NULL, "malloc");

  newHeader->width = width;
  newHeader->height = height;

  // Allocating the array of pointers to RLE rows
  newHeader->row = malloc(height * sizeof(int*));
  check(newHeader->row != NULL, "malloc");

  return newHeader;
}

/// Allocate an array to store a RLE row with n elements
static int* AllocateRLERowArray(uint32 n) {
  assert(n > 2);
  int* newArray = malloc(n * sizeof(int));
  check(newArray != NULL, "malloc");

  return newArray;
}

/// Compute the number of runs of a non-compressed (RAW) image row
static uint32 GetNumRunsInRAWRow(uint32 image_width, const uint8* RAW_row) {
  assert(image_width > 0);
  assert(RAW_row != NULL);

  // How many runs?
  uint32 num_runs = 1;
  for (uint32 i = 1; i < image_width; i++) {
    if (RAW_row[i] != RAW_row[i - 1]) {
      num_runs++;
    }
  }

  return num_runs;
}

/// Get the number of runs of a compressed RLE image row
static uint32 GetNumRunsInRLERow(const int* RLE_row) {
  assert(RLE_row != NULL);

  // Go through the RLE_row until EOR is found
  // Discard RLE_row[0], since it is a pixel color

  uint32 num_runs = 0;
  uint32 i = 1;
  while (RLE_row[i] != EOR) {
    num_runs++;
    i++;
  }

  return num_runs;
}

/// Get the number of elements of an array storing a compressed RLE image row
static uint32 GetSizeRLERowArray(const int* RLE_row) {
  assert(RLE_row != NULL);

  // Go through the array until EOR is found
  uint32 i = 0;
  while (RLE_row[i] != EOR) {
    i++;
  }

  return (i + 1);
}

/// Compress into RLE format a RAW image row
/// Allocates and returns the array storing the image row in RLE format
static int* CompressRow(uint32 image_width, const uint8* RAW_row) {
  assert(image_width > 0);
  assert(RAW_row != NULL);

  // How many runs?
  uint32 num_runs = GetNumRunsInRAWRow(image_width, RAW_row);

  // Allocate the RLE row array
  int* RLE_row = malloc((num_runs + 2) * sizeof(int));
  check(RLE_row != NULL, "malloc");

  // Go through the RAW_row
  RLE_row[0] = (int)RAW_row[0];  // Initial pixel value
  uint32 index = 1;
  int num_pixels = 1;
  for (uint32 i = 1; i < image_width; i++) {
    if (RAW_row[i] != RAW_row[i - 1]) {
      RLE_row[index++] = num_pixels;
      num_pixels = 0;
    }
    num_pixels++;
  }
  RLE_row[index++] = num_pixels;
  RLE_row[index] = EOR;  // Reached the end of the row

  return RLE_row;
}

static uint8* UncompressRow(uint32 image_width, const int* RLE_row) {
  assert(image_width > 0);
  assert(RLE_row != NULL);

  // The uncompressed row
  uint8* row = (uint8*)malloc(image_width * sizeof(uint8));
  check(row != NULL, "malloc");

  // Go through the RLE_row until EOR is found
  int pixel_value = RLE_row[0];
  uint32 i = 1;
  uint32 dest_i = 0;
  while (RLE_row[i] != EOR) {
    // For each run
    for (int aux = 0; aux < RLE_row[i]; aux++) {
      row[dest_i++] = (uint8)pixel_value;
    }
    // Next run
    i++;
    pixel_value ^= 1;
  }

  return row;
}

// Add your auxiliary functions here...

/// Image management functions

/// Create a new BW image, either BLACK or WHITE.
///   width, height : the dimensions of the new image.
///   val: the pixel color (BLACK or WHITE).
/// Requires: width and height must be non-negative, val is either BLACK or
/// WHITE.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageCreate(uint32 width, uint32 height, uint8 val) {
  assert(width > 0 && height > 0);
  assert(val == WHITE || val == BLACK);

  Image newImage = AllocateImageHeader(width, height);

  // All image pixels have the same value
  int pixel_value = (int)val;

  // Creating the image rows, each row has just 1 run of pixels
  // Each row is represented by an array of 3 elements [value,length,EOR]
  for (uint32 i = 0; i < height; i++) {
    newImage->row[i] = AllocateRLERowArray(3);
    newImage->row[i][0] = pixel_value;
    newImage->row[i][1] = (int)width;
    newImage->row[i][2] = EOR;
  }

  return newImage;
}

/// Create a new BW image, with a perfect CHESSBOARD pattern.
///   width, height : the dimensions of the new image.
///   square_edge : the lenght of the edges of the sqares making up the
///   chessboard pattern.
///   first_value: the pixel color (BLACK or WHITE) of the
///   first image pixel.
/// Requires: width and height must be non-negative, val is either BLACK or
/// WHITE.
/// Requires: for the squares, width and height must be multiples of the
/// edge lenght of the squares
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageCreateChessboard(uint32 width, uint32 height, uint32 square_edge,
                            uint8 first_value) {
    assert(width > 0 && height > 0);
    assert(square_edge > 0);
    assert(first_value == WHITE || first_value == BLACK);

    assert(width % square_edge == 0 && height % square_edge == 0); // Garantir a divisibilidade

    Image newImage = AllocateImageHeader(width, height);

    // Adicionar a memória usada pelo ImageStruct e ponteiros de linha
    InstrCount[1] += sizeof(struct image) + height * sizeof(int*);

    uint32 squares_per_row = width / square_edge;

    for (uint32 i = 0; i < height; i++) {
        // Determinar a cor inicial para esta linha
        uint8 row_start_color = first_value;
        if ((i / square_edge) % 2 != 0) {
            row_start_color ^= 1;  // Alternar a cor inicial
        }

        // Criar uma linha RAW com o padrão xadrez
        uint8* raw_row = malloc(width * sizeof(uint8));
        check(raw_row != NULL, "malloc");

        uint8 current_color = row_start_color;

        for (uint32 j = 0; j < width; j++) {
            raw_row[j] = current_color;
            // Alternar a cor a cada 'square_edge' pixels
            if ((j + 1) % square_edge == 0) {
                current_color ^= 1;  // Alternar a cor
            }
        }

        // Comprimir a linha RAW
        int* RLE_row = CompressRow(width, raw_row);
        // Adicionar a memória usada pela linha RLE
        uint32 num_elems = GetSizeRLERowArray(RLE_row);
        InstrCount[1] += num_elems * sizeof(int);
        // Contar o número de runs
        uint32 num_runs = GetNumRunsInRLERow(RLE_row);
        InstrCount[0] += num_runs;

        newImage->row[i] = RLE_row;

        free(raw_row);
    }

    return newImage;
}


/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail.
void ImageDestroy(Image* imgp) {
  assert(imgp != NULL);

  Image img = *imgp;

  for (uint32 i = 0; i < img->height; i++) {
    free(img->row[i]);
  }
  free(img->row);
  free(img);

  *imgp = NULL;
}

/// Printing on the console

/// Output the raw BW image
void ImageRAWPrint(const Image img) {
  assert(img != NULL);

  printf("width = %d height = %d\n", img->width, img->height);
  printf("RAW image:\n");

  // Print the pixels of each image row
  for (uint32 i = 0; i < img->height; i++) {
    // The value of the first pixel in the current row
    int pixel_value = img->row[i][0];
    for (uint32 j = 1; img->row[i][j] != EOR; j++) {
      // Print the current run of pixels
      for (int k = 0; k < img->row[i][j]; k++) {
        printf("%d", pixel_value);
      }
      // Switch (XOR) to the pixel value for the next run, if any
      pixel_value ^= 1;
    }
    // At current row end
    printf("\n");
  }
  printf("\n");
}

/// Output the compressed RLE image
void ImageRLEPrint(const Image img) {
  assert(img != NULL);

  printf("width = %d height = %d\n", img->width, img->height);
  printf("RLE encoding:\n");

  // Print the compressed rows information
  for (uint32 i = 0; i < img->height; i++) {
    uint32 j;
    for (j = 0; img->row[i][j] != EOR; j++) {
      printf("%d ", img->row[i][j]);
    }
    printf("%d\n", img->row[i][j]);
  }
  printf("\n");
}

/// PBM BW file operations

// See PBM format specification: http://netpbm.sourceforge.net/doc/pbm.html

// Auxiliary function
static void unpackBits(int nbytes, const uint8 bytes[], uint8 raw_row[]) {
  // bitmask starts at top bit
  int offset = 0;
  uint8 mask = 1 << (7 - offset);
  while (offset < 8) {  // or (mask > 0)
    for (int b = 0; b < nbytes; b++) {
      raw_row[8 * b + offset] = (bytes[b] & mask) != 0;
    }
    mask >>= 1;
    offset++;
  }
}

// Auxiliary function
static void packBits(int nbytes, uint8 bytes[], const uint8 raw_row[]) {
  // bitmask starts at top bit
  int offset = 0;
  uint8 mask = 1 << (7 - offset);
  while (offset < 8) {  // or (mask > 0)
    for (int b = 0; b < nbytes; b++) {
      if (offset == 0) bytes[b] = 0;
      bytes[b] |= raw_row[8 * b + offset] ? mask : 0;
    }
    mask >>= 1;
    offset++;
  }
}

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE* f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Load a raw PBM file.
/// Only binary PBM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageLoad(const char* filename) {  ///
  int w, h;
  char c;
  FILE* f = NULL;
  Image img = NULL;

  check((f = fopen(filename, "rb")) != NULL, "Open failed");
  // Parse PBM header
  check(fscanf(f, "P%c ", &c) == 1 && c == '4', "Invalid file format");
  skipComments(f);
  check(fscanf(f, "%d ", &w) == 1 && w >= 0, "Invalid width");
  skipComments(f);
  check(fscanf(f, "%d", &h) == 1 && h >= 0, "Invalid height");
  check(fscanf(f, "%c", &c) == 1 && isspace(c), "Whitespace expected");

  // Allocate image
  img = AllocateImageHeader(w, h);

  // Read pixels
  int nbytes = (w + 8 - 1) / 8;  // number of bytes for each row
  // using VLAs...
  uint8 bytes[nbytes];
  uint8 raw_row[nbytes * 8];
  for (uint32 i = 0; i < img->height; i++) {
    check(fread(bytes, sizeof(uint8), nbytes, f) == (size_t)nbytes,
          "Reading pixels");
    unpackBits(nbytes, bytes, raw_row);
    img->row[i] = CompressRow(w, raw_row);
  }

  fclose(f);
  return img;
}

/// Save image to PBM file.
/// On success, returns nonzero.
/// On failure, returns 0, and
/// a partial and invalid file may be left in the system.
int ImageSave(const Image img, const char* filename) {  ///
  assert(img != NULL);
  int w = img->width;
  int h = img->height;
  FILE* f = NULL;

  check((f = fopen(filename, "wb")) != NULL, "Open failed");
  check(fprintf(f, "P4\n%d %d\n", w, h) > 0, "Writing header failed");

  // Write pixels
  int nbytes = (w + 8 - 1) / 8;  // number of bytes for each row
  // using VLAs...
  uint8 bytes[nbytes];
  // unit8 raw_row[nbytes*8];
  for (uint32 i = 0; i < img->height; i++) {
    // UncompressRow...
    uint8* raw_row = UncompressRow(nbytes * 8, img->row[i]);
    // Fill padding pixels with WHITE
    memset(raw_row + w, WHITE, nbytes * 8 - w);
    packBits(nbytes, bytes, raw_row);
    check(fwrite(bytes, sizeof(uint8), nbytes, f) == (size_t)nbytes,
          "Writing pixels failed");
    free(raw_row);
  }

  // Cleanup
  fclose(f);
  return 0;
}

/// Information queries

/// Get image width
int ImageWidth(const Image img) {
  assert(img != NULL);
  return img->width;
}

/// Get image height
int ImageHeight(const Image img) {
  assert(img != NULL);
  return img->height;
}

/// Image comparison

int ImageIsEqual(const Image img1, const Image img2) {
  assert(img1 != NULL && img2 != NULL);

  // Verifica se as dimensões são diferentes
    if (img1->width != img2->width || img1->height != img2->height) {
        return 0; // Imagens não são iguais
    }

    // Percorre cada pixel das duas imagens
    for (int y = 0; y < img1->height; y++) {
        for (int x = 0; x < img1->width; x++) {
            if (img1->row[y][x] != img2->row[y][x]) {
                return 0; // Pixels diferentes encontrados
            }
        }
    }

    // Se passar por todos os pixels sem diferenças, as imagens são iguais
    return 1;
}


int ImageIsDifferent(const Image img1, const Image img2)
{
    assert(img1 != NULL && img2 != NULL);
    return !ImageIsEqual(img1, img2);
}

/// Boolean Operations on image pixels

/// These functions apply boolean operations to images,
/// returning a new image as a result.
///
/// Operand images are left untouched and must be of the same size.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)

Image ImageNEG(const Image img) {
  assert(img != NULL);

  uint32 width = img->width;
  uint32 height = img->height;

  Image newImage = AllocateImageHeader(width, height);

  // Directly copying the rows, one by one
  // And changing the value of row[i][0]

  for (uint32 i = 0; i < height; i++) {
    uint32 num_elems = GetSizeRLERowArray(img->row[i]);
    newImage->row[i] = AllocateRLERowArray(num_elems);
    memcpy(newImage->row[i], img->row[i], num_elems * sizeof(int));
    newImage->row[i][0] ^= 1;  // Just negate the value of the first pixel run
  }

  return newImage;
}


Image ImageAND(const Image img1, const Image img2) {
  assert(img1 != NULL && img2 != NULL);

  // COMPLETE THE CODE
  // You might consider using the UncompressRow and CompressRow auxiliary files
  // Or devise a more efficient alternative

  //contador de operações para testar complexidade
  int counter = 0;

  //reset e configuração dos counters
  InstrReset();
  InstrName[0] = "oper";

  //Garantir que as dimensões sejam iguais
  assert(img1->width == img2->width && img1->height == img2->height);

  // Criar um cabeçalho novo para a imagem resultante
  Image result = AllocateImageHeader(img1->width, img1->height);

  // Processar cada linha das imagens
  for (uint32 y = 0; y < img1->height; y++) {

    const int* rle_row1 = img1->row[y];
    const int* rle_row2 = img2->row[y];

    // Criar uma linha RAW para armazenar os resultados da operação AND
    uint8* raw_row1 = UncompressRow(img1->width, rle_row1);
    uint8* raw_row2 = UncompressRow(img2->width, rle_row2);
    uint8* raw_result = malloc(img1->width * sizeof(uint8));
    check(raw_result != NULL, "malloc");

    // Realizar a operação AND entre as linhas raw
    for (uint32 x = 0; x < img1->width; x++) {
        raw_result[x] = raw_row1[x] & raw_row2[x];
        InstrCount[0]++; //incrementa o contador a cada operação
    }

    // Comprimir a linha de resultado para o formato de compressão rle
    result->row[y] = CompressRow(img1->width, raw_result);

    // free às linhas RAW temporárias
    free(raw_row1);
    free(raw_row2);
    free(raw_result);
  }
  return result;
}
/**
Image ImageAND(const Image img1, const Image img2) {
    // Verifica se as imagens não são nulas e têm as mesmas dimensões
    assert(img1 != NULL && img2 != NULL);
    assert(ImageWidth(img1) == ImageWidth(img2) && ImageHeight(img1) == ImageHeight(img2));

    // Reset dos contadores de operações
    InstrReset();
    InstrName[0] = "Operações AND";

    // Aloca uma nova imagem para armazenar o resultado
    Image result = AllocateImageHeader(ImageWidth(img1), ImageHeight(img1));
    assert(result != NULL);

    // Itera sobre cada linha da imagem
    for (uint32 y = 0; y < ImageHeight(img1); y++) {
        const int* rle_row1 = img1->row[y];
        const int* rle_row2 = img2->row[y];
        int* rle_result = AllocateRLERowArray(ImageWidth(img1));

        int idx1 = 1, idx2 = 1, idx_res = 0;
        int len1_rest = rle_row1[idx1]; // Comprimento restante do run em rle_row1
        int len2_rest = rle_row2[idx2]; // Comprimento restante do run em rle_row2

        int current_value = rle_row1[0] & rle_row2[0]; // Valor inicial do resultado
        rle_result[idx_res++] = current_value;

        while (len1_rest > 0 && len2_rest > 0) {
            int min_len = (len1_rest < len2_rest) ? len1_rest : len2_rest;

            // Obtém os valores dos runs e realiza a operação AND
            int val1 = (idx1 == 1) ? rle_row1[0] : (rle_row1[0] ^ 1);
            int val2 = (idx2 == 1) ? rle_row2[0] : (rle_row2[0] ^ 1);
            int and_value = val1 & val2;

            if (and_value == current_value) {
                rle_result[idx_res - 1] += min_len; // Incrementa o comprimento do run atual
            } else {
                rle_result[idx_res++] = min_len;
                rle_result[idx_res++] = and_value;
                current_value = and_value;
            }

            // Incrementa o contador de operações
            InstrCount[0] += min_len;

            // Atualiza os comprimentos restantes e avança os índices, se necessário
            len1_rest -= min_len;
            len2_rest -= min_len;

            if (len1_rest == 0 && rle_row1[idx1 + 1] != EOR) {
                idx1 += 2;
                len1_rest = rle_row1[idx1];
            }
            if (len2_rest == 0 && rle_row2[idx2 + 1] != EOR) {
                idx2 += 2;
                len2_rest = rle_row2[idx2];
            }
        }

        // Marca o fim da linha resultante
        rle_result[idx_res++] = EOR;
        result->row[y] = rle_result;
    }


    return result;
}

***/
Image ImageOR(const Image img1, const Image img2) {
  assert(img1 != NULL && img2 != NULL);

  // COMPLETE THE CODE
  // You might consider using the UncompressRow and CompressRow auxiliary files
  // Or devise a more efficient alternative

  // assert das dimensões das imagens
  assert(img1->width == img2->width && img1->height == img2->height);

  uint32 width = img1->width;
  uint32 height = img1->height;

  Image newImage = AllocateImageHeader(width, height); //alocar um novo cabeçalho

  for (uint32 i = 0; i < height; i++) {
    // descomprimir as linhas 
    uint8* row1 = UncompressRow(width, img1->row[i]);
    uint8* row2 = UncompressRow(width, img2->row[i]);

    
    uint8* result_row = malloc(width * sizeof(uint8));
    check(result_row != NULL, "malloc"); // comfirmar alocação
    
    // bitwise or
    for (uint32 j = 0; j < width; j++) {
      result_row[j] = row1[j] | row2[j];
    }

    // comprimir a linha
    newImage->row[i] = CompressRow(width, result_row);

    // free às linhas
    free(row1);
    free(row2);
    free(result_row);
  }

  return newImage;
}


Image ImageXOR(Image img1, Image img2) {
  assert(img1 != NULL && img2 != NULL);

  // COMPLETE THE CODE
  // You might consider using the UncompressRow and CompressRow auxiliary files
  // Or devise a more efficient alternative

  // assert das dimensões da imagem
  assert(img1->width == img2->width && img1->height == img2->height);

  uint32 width =  img1->width;
  uint32 height = img1->height;

  Image newImage = AllocateImageHeader(width, height);

  for (uint32 i = 0; i < height; i++) {

    // descomprimir as linahs de cada uma das duas imagens
    uint8* row1 = UncompressRow(width, img1->row[i]);
    uint8* row2 = UncompressRow(width, img2->row[i]);


    uint8* result_row = malloc(width * sizeof(uint8));
    check(result_row != NULL, "malloc"); // comfirmar alocação
    //bitwise xor
    for (uint32 j = 0; j < width; j++) {
      result_row[j] = row1[j] ^ row2[j];
    }

    // comprimir as rows
    newImage->row[i] = CompressRow(width, result_row);

    // free às rows
    free(row1);
    free(row2);
    free(result_row);
  }

  return newImage;
}

/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)

/// Mirror an image = flip top-bottom.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageHorizontalMirror(const Image img) {
  assert(img != NULL);

  uint32 width = img->width;
  uint32 height = img->height;

  Image newImage = AllocateImageHeader(width, height);

// copiar a row do fundo da imagem original 
  for (uint32 i = 0; i < height; i++) {
    uint32 src_row = height - 1 - i; // indice da linha original considerando o espelhamento
    uint32 num_elems = GetSizeRLERowArray(img->row[src_row]); //nº de elementos da linha
    newImage->row[i] = AllocateRLERowArray(num_elems); //alocar espaço na memória

    // copia o conteúdo da linha comprimida da imagem original para uma nova linha
    memcpy(newImage->row[i], img->row[src_row], num_elems * sizeof(int)); 
  }

  return newImage;
}

/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageVerticalMirror(const Image img) {
  assert(img != NULL);

  uint32 width = img->width;
  uint32 height = img->height;

  Image newImage = AllocateImageHeader(width, height);

  for (uint32 i = 0; i < height; i++) {
    // descomprimir a linha
    uint8* row = UncompressRow(width, img->row[i]);

    // inversão da linha
    for (uint32 j = 0; j < width / 2; j++) {
      uint8 temp = row[j];
      row[j] = row[width - 1 - j]; // copia o valor do elemento simétrico para j
      row[width - 1 - j] = temp; // copia o valor temporário para a posição simétrica
    }

    // comprimir a row
    newImage->row[i] = CompressRow(width, row);

    free(row);
  }


  return newImage;
}

/// Replicate img2 at the bottom of imag1, creating a larger image
/// Requires: the width of the two images must be the same.
/// Returns the new larger image.
/// Ensures: The original images are not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageReplicateAtBottom(const Image img1, const Image img2) {
  assert(img1 != NULL && img2 != NULL);
  //assert das dimensões
  assert(img1->width == img2->width);

  uint32 new_width = img1->width;
  uint32 new_height = img1->height + img2->height; //new_height é a soma das height originais de cada imagem

  Image newImage = AllocateImageHeader(new_width, new_height);
// copiar as rows da imagem 1
  for (uint32 i = 0; i < img1->height; i++) {
    uint32 num_elems = GetSizeRLERowArray(img1->row[i]);
    newImage->row[i] = AllocateRLERowArray(num_elems);

    memcpy(newImage->row[i], img1->row[i], num_elems * sizeof(int));
  }

  //copiar as rows da imagem 2
  for (uint32 i = 0; i < img2->height; i++) {
    uint32 num_elems = GetSizeRLERowArray(img2->row[i]);
    newImage->row[img1->height + i] = AllocateRLERowArray(num_elems);
    memcpy(newImage->row[img1->height + i], img2->row[i], num_elems * sizeof(int));
  }

  return newImage;
}

/// Replicate img2 to the right of imag1, creating a larger image
/// Requires: the height of the two images must be the same.
/// Returns the new larger image.
/// Ensures: The original images are not modified.
///                
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
Image ImageReplicateAtRight(const Image img1, const Image img2) {
  assert(img1 != NULL && img2 != NULL);
  assert(img1->height == img2->height);

  uint32 new_width = img1->width + img2->width;
  uint32 new_height = img1->height;

  Image newImage = AllocateImageHeader(new_width, new_height);

  // COMPLETE THE CODE
  for (uint32 i = 0; i < new_height; i++) {
    // descomprimir as rows
    uint8* row1 = UncompressRow(img1->width, img1->row[i]);
    uint8* row2 = UncompressRow(img2->width, img2->row[i]);

    // concatenação das rows
    uint8* result_row = malloc(new_width * sizeof(uint8));
    check(result_row != NULL, "malloc"); //confirmar malloc

    //Copia os primeiros img1-> width elementos de row1 para o inicio de result_row
    memcpy(result_row, row1, img1->width * sizeof(uint8));

    // Destino começa após os primeiros img1->width elementos
    memcpy(result_row + img1->width, row2, img2->width * sizeof(uint8));

    // Ccomprimir a row resultante
    newImage->row[i] = CompressRow(new_width, result_row);

    // free às rows
    free(row1);
    free(row2);
    free(result_row);
  }

  return newImage;
}
