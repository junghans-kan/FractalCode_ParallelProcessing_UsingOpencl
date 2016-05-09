
__kernel void mandelbrot_kernel(__global int *R, __global int *G, __global int
*B, 
								int count_max, float x_max, float x_min, 
								float y_max, float y_min)
{
  __global int (*r)[n] = (__global int (*)[n])R;
  __global int (*g)[n] = (__global int (*)[n])G;
  __global int (*b)[n] = (__global int (*)[n])B;

  int c;
  int i, j, k;
  i = get_global_id(0);

  float x;
  float x1;
  float x2;
  float y;
  float y1;
  float y2;

  // Carry out the iteration for each pixel, determining COUNT.
  if( i >= m ) return;

  for ( j = 0; j < n; j++ )
   {
      x = ( ( float ) (     j - 1 ) * x_max  
          + ( float ) ( m - j     ) * x_min )
          / ( float ) ( m     - 1 );

      y = ( ( float ) (     i - 1 ) * y_max  
          + ( float ) ( n - i     ) * y_min )
          / ( float ) ( n     - 1 );

      int count = 0;

      x1 = x;
      y1 = y;

      for ( k = 1; k <= count_max; k++ )
      {
        x2 = x1 * x1 - y1 * y1 + x;
        y2 = 2.0 * x1 * y1 + y;

        if ( x2 < -2.0 || 2.0 < x2 || y2 < -2.0 || 2.0 < y2 )
        {
          count = k;
          break;
        }
        x1 = x2;
        y1 = y2;
      }

      if ( ( count % 2 ) == 1 )
      {
        r[i][j] = 255;
        g[i][j] = 255;
        b[i][j] = 255;
      }
      else
      {
        c = ( int ) ( 255.0 * (float)sqrt ( (float)sqrt ( (float)sqrt (
          ( ( float ) ( count ) / ( float ) ( count_max ) ) ) ) ) );
        r[i][j] = 3 * c / 5;
        g[i][j] = 3 * c / 5;
        b[i][j] = c;
      }
   } 
}
