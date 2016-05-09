#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_M   2048
#define DEFAULT_N   2048

void mandelbrot(int m, int n);
void julia(int w, int h);


int main(int argc, char *argv[]) {
  int m = DEFAULT_M;
  int n = DEFAULT_N;

  const char *file_name = "size.txt";
  FILE *file = fopen(file_name, "r");
  if (file == NULL) {
    fprintf(stderr, "%s does not exist. "
            "The default size (%d x %d) is used.\n",
            file_name, m, n);
  } else {
    char buf[128];

    if (fgets(buf, sizeof(buf), file) != NULL) {
      m = (int)atol(buf);
    }
    if (fgets(buf, sizeof(buf), file) != NULL) {
      n = (int)atol(buf);
    }

    fclose(file);
  }

  printf("1. MANDELBROT SET\n\n");
  mandelbrot(m, n);
  printf("\n");

  printf("2. JULIA SET\n\n");
  julia(m, n);

  return EXIT_SUCCESS;
}

