#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "timers.h"
#include <CL/cl.h>
#include "cl_util.h"

#define COUNT_MAX   5000
#define MIN(x,y)    ((x) < (y) ? (x) : (y))


void mandelbrot(int m, int n)
{
  cl_platform_id   *platform;
  cl_device_type   dev_type = CL_DEVICE_TYPE_GPU;
  cl_device_id     *devs = NULL;
  cl_context       context;
  cl_command_queue *cmd_queues;
  cl_program       program;
  cl_kernel        *kernels;
  cl_mem           *mem_R;
  cl_mem		   *mem_G;
  cl_mem		   *mem_B;
  cl_int           err;
  cl_uint          num_platforms;
  cl_uint          num_devs = 0;
  cl_event		   *ev_kernels;

	
		
  int count_max = COUNT_MAX;
  int i, j, jhi, jlo;
  char *output_filename = "mandelbrot.ppm";
  FILE *output_unit;
  double wtime;

  float x_max =   1.25;
  float x_min = - 2.25;
//  float x;
//  float x1;
//  float x2;
  float y_max =   1.75;
  float y_min = - 1.75;
  //float y;
  //float y1;
  //float y2;

  size_t size_color;

  size_color = sizeof(int) * m * n;

  int (*r)[n] = (int (*)[n])calloc(m * n, sizeof(int));
  int (*g)[n] = (int (*)[n])calloc(m * n, sizeof(int));
  int (*b)[n] = (int (*)[n])calloc(m * n, sizeof(int));

  printf( "  Sequential C version\n" );
  printf( "\n" );
  printf( "  Create an ASCII PPM image of the Mandelbrot set.\n" );
  printf( "\n" );
  printf( "  For each point C = X + i*Y\n" );
  printf( "  with X range [%g,%g]\n", x_min, x_max );
  printf( "  and  Y range [%g,%g]\n", y_min, y_max );
  printf( "  carry out %d iterations of the map\n", count_max );
  printf( "  Z(n+1) = Z(n)^2 + C.\n" );
  printf( "  If the iterates stay bounded (norm less than 2)\n" );
  printf( "  then C is taken to be a member of the set.\n" );
  printf( "\n" );
  printf( "  An ASCII PPM image of the set is created using\n" );
  printf( "    M = %d pixels in the X direction and\n", m );
  printf( "    N = %d pixels in the Y direction.\n", n );

  timer_init();
  timer_start(0);

  // Platform
  err = clGetPlatformIDs(0, NULL, &num_platforms);
  CHECK_ERROR(err);
  if (num_platforms == 0) {
    fprintf(stderr, "[%s:%d] ERROR: No OpenCL platform\n", __FILE__,__LINE__);
    exit(EXIT_FAILURE);
  }

  printf("Number of platforms: %u\n", num_platforms);
  platform = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platforms);
  err = clGetPlatformIDs(num_platforms, platform, NULL);
  CHECK_ERROR(err);
  
  // Device
  for (i = 0; i < num_platforms; i++) {
    err = clGetDeviceIDs(platform[i], dev_type, 0, NULL, &num_devs);
    if (err != CL_DEVICE_NOT_FOUND) CHECK_ERROR(err);
	num_devs = 1; //**
    if (num_devs >= 1)
	{
		devs = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devs);

		err = clGetDeviceIDs(platform[i], dev_type, num_devs, devs, NULL);
		break;
	}
  }
  if ( devs == NULL || num_devs < 1) {
    fprintf(stderr, "[%s:%d] ERROR: No device\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }

  for( i = 0; i < num_devs; ++i ) {
	printf("dev[%d] : ", i);
  	print_device_name(devs[i]);
  }

  // Context
  context = clCreateContext(NULL, num_devs, devs, NULL, NULL, &err);
  CHECK_ERROR(err);

  // Command queue
  cmd_queues = (cl_command_queue*)malloc(sizeof(cl_command_queue)*num_devs);
  for( i = 0; i < num_devs; ++i) {
	cmd_queues[i] = clCreateCommandQueue(context, devs[i], 0, &err);
  	CHECK_ERROR(err);
  }

  // Create a program.
  size_t source_len;
  char *source_code = get_source_code("./mandelbrot_kernel.cl", &source_len);
  program = clCreateProgramWithSource(context,
                                      1,
                                      (const char **)&source_code,
                                      &source_len,
                                      &err);
  free(source_code);
  CHECK_ERROR(err);

  // Build the program.
  char build_opts[200];
  sprintf(build_opts, "-Dm=%d -Dn=%d -Dnum_devs=%d", m, n, num_devs);
  err = clBuildProgram(program, num_devs, devs, build_opts, NULL, NULL);
  if (err != CL_SUCCESS) {
    print_build_log(program, devs[0]);
    CHECK_ERROR(err);
  }
  
  // Kernel
  kernels = (cl_kernel*)malloc(sizeof(cl_kernel)*num_devs);
  for (i = 0; i < num_devs; i++) {
	  kernels[i] = clCreateKernel(program, "mandelbrot_kernel", NULL);
  }
 
  // Buffers  
  mem_R = (cl_mem*)malloc(sizeof(cl_mem)*num_devs);
  mem_G = (cl_mem*)malloc(sizeof(cl_mem)*num_devs);
  mem_B = (cl_mem*)malloc(sizeof(cl_mem)*num_devs);

  for(i = 0; i < num_devs; i++) {
	  mem_R[i] = clCreateBuffer(context, CL_MEM_READ_WRITE,
                         size_color / num_devs, NULL, NULL);
	  mem_G[i] = clCreateBuffer(context, CL_MEM_READ_WRITE,
                         size_color / num_devs, NULL, NULL);
	  mem_B[i] = clCreateBuffer(context, CL_MEM_READ_WRITE,
                         size_color / num_devs, NULL, NULL);
  }

/*
  // Write to Buffers
  for(i = 0; i < num_devs; i++) {
  	clEnqueueWriteBuffer(cmd_queues[i],
                         mem_CHECK[i], 
                         CL_FALSE, 0,
                         size_CHECK / num_devs,
                         (CHECK + (N / num_devs) * i),
                         0, NULL, NULL);
  }
*/

  // Set the arguments.
  for(i = 0; i < num_devs; i++) {
//	  flag = i * (m * n / num_devs);
  	clSetKernelArg(kernels[i], 0, sizeof(cl_mem), (void*) &mem_R[i]);
	clSetKernelArg(kernels[i], 1, sizeof(cl_mem), (void*) &mem_G[i]);
  	clSetKernelArg(kernels[i], 2, sizeof(cl_mem), (void*) &mem_B[i]);

	clSetKernelArg(kernels[i], 3, sizeof(int), &count_max);
	clSetKernelArg(kernels[i], 4, sizeof(float), &x_max);
	clSetKernelArg(kernels[i], 5, sizeof(float), &x_min);
	clSetKernelArg(kernels[i], 6, sizeof(float), &y_max);
	clSetKernelArg(kernels[i], 7, sizeof(float), &y_min);
  }

  // Enqueue the kernel.
  size_t lws[1] = {256};
  size_t gws[1] = { m * n /num_devs };
  gws[0] = (size_t)ceil((double)m * n / lws[0]) * lws[0];
  ev_kernels = (cl_event*)malloc(sizeof(cl_event)*num_devs);
  for(i = 0; i < num_devs; i++) {
 	 err = clEnqueueNDRangeKernel(cmd_queues[i], kernels[i], 1, NULL, gws, lws, 0, NULL, &ev_kernels[i]);
  	 CHECK_ERROR(err);
  }

  // Read the result.
  for(i = 0; i < num_devs; i++) {
  	err = clEnqueueReadBuffer(cmd_queues[i],
                            mem_R[i],
                            CL_TRUE, 0,
                            size_color / num_devs,
                            r,
                            1, &ev_kernels[i], NULL);
  	err = clEnqueueReadBuffer(cmd_queues[i],
                            mem_G[i],
                            CL_TRUE, 0,
                            size_color / num_devs,
                            g,
							1, &ev_kernels[i], NULL);
   	err = clEnqueueReadBuffer(cmd_queues[i],
                            mem_B[i],
                            CL_TRUE, 0,
							size_color / num_devs,
							b,
                            1, &ev_kernels[i], NULL);
  }

  // Release
  for( i = 0; i < num_devs; ++i ) {
  clFinish(cmd_queues[i]); 
  clReleaseMemObject(mem_R[i]);
  clReleaseMemObject(mem_G[i]);
  clReleaseMemObject(mem_B[i]);
  clReleaseKernel(kernels[i]);
  clReleaseCommandQueue(cmd_queues[i]);
  clReleaseEvent(ev_kernels[i]);
  }
  clReleaseProgram(program);
  clReleaseContext(context);
  free(mem_R);
  free(mem_G);
  free(mem_B);
  free(cmd_queues);
  free(kernels);
  free(devs);
  free(ev_kernels);
  free(platform);

  timer_stop(0);
  wtime = timer_read(0);
  printf( "\n" );
  printf( "  Time = %lf seconds.\n", wtime );

  // Write data to an ASCII PPM file.
  output_unit = fopen( output_filename, "wt" );

  fprintf( output_unit, "P3\n" );
  fprintf( output_unit, "%d  %d\n", n, m );
  fprintf( output_unit, "%d\n", 255 );
  for ( i = 0; i < m; i++ )
  {
    for ( jlo = 0; jlo < n; jlo = jlo + 4 )
    {
      jhi = MIN( jlo + 4, n );
      for ( j = jlo; j < jhi; j++ )
      {
        fprintf( output_unit, "  %d  %d  %d", r[i][j], g[i][j], b[i][j] );
      }
      fprintf( output_unit, "\n" );
    }
  }

  fclose( output_unit );
  printf( "\n" );
  printf( "  Graphics data written to \"%s\".\n\n", output_filename );

  // Terminate.
  free(r);
  free(g);
  free(b);
}

