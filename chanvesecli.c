/**
 * @file chanvesecli.c
 * @brief Chan-Vese image segmentation IPOL demo
 * @author Pascal Getreuer <getreuer@gmail.com>
 *
 * Copyright (c) 2011-2012, Pascal Getreuer
 * All rights reserved.
 *
 * This program is free software: you can use, modify and/or
 * redistribute it under the terms of the simplified BSD License. You
 * should have received a copy of this license along this program. If
 * not, see <http://www.opensource.org/licenses/bsd-license.html>.
 */

/**
 * @mainpage
 * @verbinclude readme.txt
 */

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <grvy.h>
#include "cliio.h"
#include "chanvese.h"
#include "gifwrite.h"
#include "rgb2ind.h"

#define ROUNDCLAMP(x)   ((x < 0) ? 0 : \
    ((x > 1) ? 255 : (uint8_t)floor(255.0*(x) + 0.5)))

#ifdef __GNUC__
    /** @brief Macro for the unused attribue GNU extension */
    #define ATTRIBUTE_UNUSED __attribute__((unused))
#else
    #define ATTRIBUTE_UNUSED
#endif

/** @brief Program parameters struct */
typedef struct
{
    /** @brief Input file name */
    const char *InputFile;
    /** @brief Animation output file name */
    const char *OutputFile;
    /** @brief Binary output file name */
    const char *OutputFile2;
    /** @brief Quality for saving JPEG images (0 to 100) */
    int JpegQuality;
  
    /** @brief Level set */
    image Phi;
    /** @brief ChanVese options object */
    chanveseopt *Opt;
    
    int IterPerFrame;
} programparams;

/** @brief Plotting parameters struct */
typedef struct
{
    const num *Image;
    unsigned char *Plot;
    int *Delays;
    int IterPerFrame; int NumFrames;
} plotparam;


static void PrintHelpMessage()
{
    puts("chanvese, P. Getreuer 2011-2012\n"
    "Chan-Vese segmentation IPOL demo\n\n"
    "Usage: chanvese paramFile \n\n"
    "where paramFile is a text file containing:\n"
    "   inputImage = <filename> \n"
    "   outputImage = <filename> \n"
    "   outputAnimation = <filename> \n"
    "where \"inputImage\" and \"outputImage\" are "
    READIMAGE_FORMATS_SUPPORTED " files\n"
    "and \"outputAnimation\" is a GIF file.\n");
    puts("Optional Parameters\n");
    puts("   mu = <number>           length penalty (default 0.25)");
    puts("   nu = <number>           area penalty (default 0.0)");
    puts("   lambda1 = <number>      fit weight inside the curve (default 1.0)");
    puts("   lambda2 = <number>      fit weight outside the curve (default 1.0)");
    puts("   phi0 = <filename>           read initial level set from an image or text file");
    puts("   tol = <number>          convergence tolerance (default 1e-3)");
    puts("   maxIter = <number>      maximum number of iterations (default 500)");
    puts("   dt = <number>           time step (default 0.5)\n");
    puts("   iterPerFrame = <number> iterations per frame (default 10)\n");
#ifdef LIBJPEG_SUPPORT
    puts("   jpegQuality = <number>  Quality for saving JPEG images (0 to 100)\n");
#endif
    puts("Example:\n"
#ifdef LIBPNG_SUPPORT
    "   chanvese tol:1e-5 mu:0.5 input.png animation.gif final.png\n");
#else
    "   chanvese tol:1e-5 mu:0.5 input.bmp animation.gif final.bmp\n");
#endif
    exit(1);
}


static int PlotFun(int State, int Iter, num Delta,
    const num *c1, const num *c2, const num *Phi,
    int Width, int Height, int NumChannels, void *ParamPtr);
static int ParseParam(programparams *Param, int argc, const char *argv[]);
static int PhiRescale(image *Phi);


int WriteBinary(image Phi, const char *File)
{
    unsigned char *Temp = NULL;
    const int NumPixels = Phi.Width*Phi.Height;
    int i, Success;
    
    if(!(Temp = (unsigned char *)malloc(Phi.Width*Phi.Height)))
        return 0;
    
    for(i = 0; i < NumPixels; i++)
        Temp[i] = (Phi.Data[i] >= 0) ? 255 : 0;
    
    Success = WriteImage(Temp, Phi.Width, Phi.Height, File,
        IMAGEIO_U8 | IMAGEIO_GRAYSCALE, 0);
    
    free(Temp);
    return Success;
}


int WriteAnimation(plotparam *PlotParam, int Width, int Height,
    const char *OutputFile)
{
    const int NumPixels = Width*Height;
    unsigned char *PlotInd = NULL;
    unsigned char *Palette = NULL;
    unsigned char **PlotIndFrames = NULL;
    int Frame, Success = 0;
    
    if(!(PlotInd = (unsigned char *)malloc(Width*Height*PlotParam->NumFrames))
        || !(Palette = (unsigned char *)calloc(3*256, 1))
        || !(PlotIndFrames = (unsigned char **)
            malloc(sizeof(unsigned char *)*PlotParam->NumFrames)))
        goto Catch;
    
    for(Frame = 0; Frame < PlotParam->NumFrames; Frame++)
        PlotIndFrames[Frame] = PlotInd + NumPixels*Frame;
    
    /* Quantize colors for GIF */
    if(!Rgb2Ind(PlotInd, Palette, 255, PlotParam->Plot,
        Width*Height*PlotParam->NumFrames))
        goto Catch;
    
    /* Optimize animation */
    FrameDifference(PlotIndFrames, Width, Height, PlotParam->NumFrames, 255);
    
    /* Write the output animation */
    if(!GifWrite(PlotIndFrames, Width, Height, PlotParam->NumFrames,
        Palette, 256, 255, PlotParam->Delays, OutputFile))
    {
        fprintf(stderr, "Error writing \"%s\".\n", OutputFile);
        goto Catch;
    }
    else
        printf("Output written to \"%s\".\n", OutputFile);
    
    Success = 1;
Catch:
    if(PlotIndFrames)
        free(PlotIndFrames);
    if(Palette)
        free(Palette);
    if(PlotInd)
        free(PlotInd);
    return Success;
}


int main(int argc, char *argv[])
{
    programparams Param;
    plotparam PlotParam;
    image f = NullImage;
    num c1[3], c2[3];
    int Status = 1;
    
    PlotParam.Plot = NULL;
    PlotParam.Delays = NULL;
    
    if(!ParseParam(&Param, argc, (const char **)argv))
        goto Catch;
    
    /* Read the input image */
    if(!ReadImageObj(&f, Param.InputFile))
        goto Catch;
    
    if(Param.Phi.Data &&
        (f.Width != Param.Phi.Width || f.Height != Param.Phi.Height))
    {
        fprintf(stderr, "Size mismatch: "
            "phi0 (%dx%d) does not match image size (%dx%d).\n",
            Param.Phi.Width, Param.Phi.Height, f.Width, f.Height);
        goto Catch;
    }
    
    PlotParam.Image = f.Data;
    PlotParam.IterPerFrame = Param.IterPerFrame;
    PlotParam.NumFrames = 0;
    
    ChanVeseSetPlotFun(Param.Opt, PlotFun, (void *)&PlotParam);
    
    printf("Segmentation parameters\n");
    printf("f         : [%d x %d %s]\n",
        f.Width, f.Height, (f.NumChannels == 1) ? "grayscale" : "RGB");
    printf("phi0      : %s\n", (Param.Phi.Data) ? "custom" : "default");
    ChanVesePrintOpt(Param.Opt);
#ifdef NUM_SINGLE
    printf("datatype  : single precision float\n");
#else
    printf("datatype  : double precision float\n");
#endif
    printf("\n");
    
    if(!Param.Phi.Data)
    {
        if(!AllocImageObj(&Param.Phi, f.Width, f.Height, 1))
        {
            fprintf(stderr, "Out of memory.");
            goto Catch;
        }
        
        ChanVeseInitPhi(Param.Phi.Data, Param.Phi.Width, Param.Phi.Height);
    }

    /* Perform the segmentation */
    if(!ChanVese(Param.Phi.Data, f.Data,
        f.Width, f.Height, f.NumChannels, Param.Opt))
    {
        fprintf(stderr, "Error in ChanVese.");
        goto Catch;
    }
    
    /* Compute the final region averages */
    RegionAverages(c1, c2, Param.Phi.Data, f.Data,
        f.Width, f.Height, f.NumChannels);
    
    printf("\nRegion averages\n");
    
    if(f.NumChannels == 1) printf("c1        : %.4f\nc2        : %.4f\n\n", c1[0], c2[0]);
    else if(f.NumChannels == 3)
        printf("c1        : (%.4f, %.4f, %.4f)\nc2        : (%.4f, %.4f, %.4f)\n\n",
            c1[0], c1[1], c1[2], c2[0], c2[1], c2[2]);
    
    if(Param.OutputFile2 && !WriteBinary(Param.Phi, Param.OutputFile2))
        goto Catch;
        
    if(!WriteAnimation(&PlotParam, f.Width, f.Height, Param.OutputFile))
        goto Catch;
    
    Status = 0;
Catch:
    if(PlotParam.Plot)
        free(PlotParam.Plot);
    if(PlotParam.Delays)
        free(PlotParam.Delays);
    FreeImageObj(Param.Phi);
    FreeImageObj(f);
    ChanVeseFreeOpt(Param.Opt);
    return Status;
}


/* Plot callback function */
static int PlotFun(int State, int Iter, ATTRIBUTE_UNUSED num Delta,
    ATTRIBUTE_UNUSED const num *c1, ATTRIBUTE_UNUSED const num *c2,
    const num *Phi, int Width, int Height,
    ATTRIBUTE_UNUSED int NumChannels, void *ParamPtr)
{
    const int NumPixels = Width*Height;
    plotparam *PlotParam = (plotparam *)ParamPtr;
    unsigned char *Plot, *Temp = NULL;
    int *Delays = NULL;
    num Red, Green, Blue, Alpha;
    long i;
    int x, y, Edge, NumFrames = PlotParam->NumFrames, Success = 0;
    int il, ir, iu, id;
    
    switch(State)
    {
    case 0:
        /* We print to stderr so that messages are displayed on the console
           immediately, during the TvRestore computation.  If we use stdout,
           messages might be buffered and not displayed until after TvRestore
           completes, which would defeat the point of having this real-time
           plot callback. */
        if(NumChannels == 1)
            fprintf(stderr, "   Iteration %4d     Delta %7.4f     c1 = %6.4f     c2 = %6.4f\r",
                Iter, Delta, *c1, *c2);
        else
            fprintf(stderr, "   Iteration %4d     Delta %7.4f\r", Iter, Delta);
        break;
    case 1: /* Converged successfully */
        fprintf(stderr, "Converged in %d iterations.                                            \n",
            Iter);
        break;
    case 2: /* Maximum iterations exceeded */
        fprintf(stderr, "Maximum number of iterations exceeded.                                 \n");
        break;
    }
    
    if(State == 0 && (Iter % PlotParam->IterPerFrame) > 0)
        return 1;
    
    if(!(Plot = (unsigned char *)realloc(PlotParam->Plot,
        3*Width*Height*(PlotParam->NumFrames + 1)))
        || !(Delays = (int *)realloc(PlotParam->Delays,
        sizeof(int)*(PlotParam->NumFrames + 1)))
        || !(Temp = (unsigned char *)malloc(Width*Height)))
    {
        fprintf(stderr, "Out of memory.\n");
        goto Catch;
    }
    
    PlotParam->Plot = Plot;
    PlotParam->Delays = Delays;
    Plot += 3*NumPixels*PlotParam->NumFrames;
    
    for(y = 0, i = 0; y < Height; y++)
        for(x = 0; x < Width; x++, i++)
        {
            if(Phi[i] >= 0 &&
                ((x > 0          && Phi[i - 1] < 0)
                    || (x + 1 < Width  && Phi[i + 1] < 0)
                    || (y > 0          && Phi[i - Width] < 0)
                    || (y + 1 < Height && Phi[i + Width] < 0)))
                Edge = 1;       /* Inside the curve, on the edge */
            else
                Edge = 0;
            
            Temp[i] = Edge;
        }
        
    for(y = 0, i = 0; y < Height; y++)
    {
        iu = (y == 0) ? 0 : -Width;
        id = (y == Height - 1) ? 0 : Width;
            
        for(x = 0; x < Width; x++, i++)
        {
            il = (x == 0) ? 0 : -1;
            ir = (x == Width - 1) ? 0 : 1;
            
            Red = PlotParam->Image[i];
            Green = PlotParam->Image[i + NumPixels];
            Blue = PlotParam->Image[i + 2*NumPixels];
            
            Red = 0.95f*Red;
            Green = 0.95f*Green;
            Blue = 0.95f*Blue;
            
            i = x + Width*y;
            Alpha = (4*Temp[i]
                + Temp[i + ir] + Temp[i + il]
                + Temp[i + id] + Temp[i + iu])/4.0f;
                
            if(Alpha > 1)
                Alpha = 1;
            
            Red = (1 - Alpha)*Red;
            Green = (1 - Alpha)*Green;
            Blue = (1 - Alpha)*Blue + Alpha;
            
            Plot[3*i + 0] = ROUNDCLAMP(Red);
            Plot[3*i + 1] = ROUNDCLAMP(Green);
            Plot[3*i + 2] = ROUNDCLAMP(Blue);
        }
    }
    
    PlotParam->Delays[NumFrames] = (State == 0) ? 12 : 120;
    PlotParam->NumFrames++;
    
    Success = 1;
Catch:
    if(Temp)
        free(Temp);
    return Success;
}


static int ParseParam(programparams *Param, int argc, const char *argv[])
{

    int igot;
    float mu, nu, lambda1, lambda2, tol, dt;
    int maxIter, iterPerFrame, jpegQuality;
    char *inputImage, *outputImage, *outputAnimation;  
    const char *paramFile; 


    if(!(Param->Opt = ChanVeseNewOpt()))
    {
        fprintf(stderr, "Out of memory.\n");
        return 0;
    }
        
    if(argc != 2)
    {
        PrintHelpMessage();
        return 0;
    }

    paramFile = argv[1];

    /* Initialize file to read */
    igot = grvy_input_fopen(paramFile);    
    if(!igot)
      exit(1);

    /* Read variables and echo */
    
    if(grvy_input_fread_float("mu",&mu))
      printf("--> %-10s = %f\n","mu",mu);
    
    if(grvy_input_fread_char("inputImage",&inputImage)) 
      printf("--> %-10s = %s\n","inputImage",inputImage);
   
    if(grvy_input_fread_char("outputAnimation",&outputAnimation))
      printf("--> %-10s = %s\n","outputAnimation",outputAnimation);
   
    if(grvy_input_fread_char("outputImage",&outputImage))
      printf("--> %-10s = %s\n","outputImage",outputImage);
   
    grvy_log_setlevel(GRVY_NOLOG);
    grvy_log_setlevel(GRVY_INFO);
    
    grvy_input_register_float("nu",0.0);
    grvy_input_register_float("lambda1",1.0);
    grvy_input_register_float("lambda2",1.0);
    grvy_input_register_char("phi0","NULL");
    grvy_input_register_float("tol",1e-3);
    grvy_input_register_int("maxIter",500);
    grvy_input_register_float("dt",0.5);
    grvy_input_register_int("iterPerFrame",10);
    grvy_input_register_int("jpegQuality",85);

    /* Dump file to stdout */
    printf("\n ------ Full Dump ------\n\n");
    grvy_input_fdump();
    printf("\n ---- End Full Dump ----\n\n");

    /* Dump to file */
    printf("\n ------ Full Dump to %s ------\n\n", "param.out");
    grvy_input_fdump_file("% ","param.out");
    printf("\n ------ End Full Dump ------\n\n");


    if(grvy_input_register_get_float("nu",&nu))
      printf("registered float   = %f\n",nu);

    if(grvy_input_register_get_float("lambda1",&lambda1))
      printf("registered float   = %f\n",lambda1);

    if(grvy_input_register_get_float("lambda2",&lambda2))
      printf("registered float   = %f\n",lambda2);

   /* if(grvy_input_register_get_char("phi0",NULL))
      printf("registered float   = %f\n",NULL);
   */

    if(grvy_input_register_get_float("tol",&tol))
      printf("registered float   = %f\n",tol);

    if(grvy_input_register_get_int("maxIter",&maxIter))
      printf("registered int   = %i\n",maxIter);

    if(grvy_input_register_get_float("dt",&dt))
      printf("registered float   = %f\n",dt);

    if(grvy_input_register_get_int("iterPerFrame",&iterPerFrame))
      printf("registered int   = %i\n",iterPerFrame);

    if(grvy_input_register_get_int("jpegQuality",&jpegQuality))
      printf("registered int   = %i\n",jpegQuality);

    /* Read in variable that has registered default but 
     may not be in param file */
    
    if(grvy_input_fread_float("nu",&nu))
      printf("fread_float: nu = %f\n",nu);

    if(grvy_input_fread_float("lambda1",&lambda1))
      printf("fread_float: lambda1 = %f\n",lambda1);

    if(grvy_input_fread_float("lambda2",&lambda2))
      printf("fread_float: lambda2 = %f\n",lambda2);

    if(grvy_input_fread_float("tol",&tol))
      printf("fread_float: tol = %f\n",tol);

    if(grvy_input_fread_int("maxIter",&maxIter))
      printf("fread_int: maxIter = %i\n",maxIter);

    if(grvy_input_fread_float("dt",&dt))
      printf("fread_float: dt = %f\n",dt);

    if(grvy_input_fread_int("iterPerFrame",&iterPerFrame))
      printf("fread_int: iterPerFrame = %i\n",iterPerFrame);

    if(grvy_input_fread_int("jpegQuality",&jpegQuality))
      printf("fread_int: jpegQuality = %i\n",jpegQuality);
 
    grvy_input_fclose();

/*
    const char *Option, *Value;
    num NumValue;
    char TokenBuf[256];
    int k, kread, Skip;
*/


    
    /* Set parameters */
    Param->InputFile = inputImage;
    Param->OutputFile = outputAnimation;
    Param->OutputFile2 = outputImage;
    Param->JpegQuality = jpegQuality;
    Param->Phi = NullImage;
    //Param->Opt = NULL;
    Param->IterPerFrame = iterPerFrame;
    ChanVeseSetTol(Param->Opt,tol);
    ChanVeseSetMu(Param->Opt,mu); 
    ChanVeseSetNu(Param->Opt,nu); 
    ChanVeseSetLambda1(Param->Opt,lambda1); 
    ChanVeseSetLambda2(Param->Opt,lambda2); 
    ChanVeseSetDt(Param->Opt,dt); 

    return 1;
}


/* If phi is read from an image file, this function is called to rescale
   it from the range [0,1] to [-10,10].  */
static int PhiRescale(image *Phi)
{
    const long NumEl = ((long)Phi->Width) * ((long)Phi->Height);
    num *Data = Phi->Data;
    long n;

    for(n = 0; n < NumEl; n++)
        Data[n] = 4*(2*Data[n] - 1);

    return 1;
}

