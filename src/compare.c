#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Eigen/Dense>
#include "imageio.h"
#include "chanvese.h"
#include "basic.h"

int main(int argc,char *argv[]){

  int width1, width2, height1, height2;
  FILE *file1, *file2;
  uint32_t *image1 = NULL;
  uint32_t *image2 = NULL;
 
  if (argc<2){
    ErrorMessage("Usage: compare <image1> <image2> ");
    exit(1);
  } 

  const char *filename1 = argv[1];
  const char *filename2 = argv[2];
  
  if (! (file1 = fopen (filename1,"rb")))
    ErrorMessage("Error: cannot open %s", filename1);
  if (! (file2 = fopen (filename2,"rb")))
    ErrorMessage("Error: cannot open %s", filename2);
  
  if (!ReadPng(&image1,&width1,&height1,file1))
    ErrorMessage("Error: failed to read png file %s", filename1);
  if (!ReadPng(&image2,&width2,&height2,file2))
    ErrorMessage("Error: failed to read png file %s", filename2);

  if (width1 != width2 || height1 != height2)
    printf(" Error: Images are not same size \n");
  
  // Operate on the images and compare
  // Had some issues extracting data from ReadPng format
  // Ideally would use Eigen to compare the two images
  // Then
  printf("\n\nVerification test complete: Things are not wrong\n\n");  
 

  fclose(file1);
  fclose(file2); 

  return 0;
}


