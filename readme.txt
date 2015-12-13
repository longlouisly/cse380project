
== Compiling on Stampede ==
$ module load gcc/4.7.1 boost grvy
$ make


== Program Usage ==

This source code produces a command line program chanvese, which performs
Chan-Vese active contours segmentation.

Usage: chanvese <paramFile> 

where paramFile is a text file containing:
   inputImage = <filename> 
   outputImage = <filename> 
   outputAnimation = <filename> 
where "inputImage" and "outputImage" are BMP/JPEG/PNG/TIFF files
and "outputAnimation" is a GIF file.

Optional Parameters

   mu = <number>           length penalty (default 0.25)
   nu = <number>           area penalty (default 0.0)
   lambda1 = <number>      fit weight inside the curve (default 1.0)
   lambda2 = <number>      fit weight outside the curve (default 1.0)
   phi0 = <filename>           read initial level set from an image or text file
   tol = <number>          convergence tolerance (default 1e-3)
   maxIter = <number>      maximum number of iterations (default 500)
   dt = <number>           time step (default 0.5)

   iterPerFrame = <number> iterations per frame (default 10)

Example can be found in example/example.sh

The chanvese program prints detailed usage information when executed
without arguments or "--help".


