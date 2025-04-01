#include<stdio.h>
#include<pthread.h>
#include<time.h>

#define WIDTH 1920
#define HEIGHT 1080
#define MAX_ITER 1000

#define NUM_THREADS 24


const double xmin = -2.0, xmax = 1.0;
const double ymin = -1.5, ymax = 1.5;

double map_pixel_to_real(int px) {
    return xmin + (xmax - xmin) * ((double)px / (WIDTH - 1));
}

double map_pixel_to_imaginary(int py) {
    return ymin + (ymax - ymin) * ((double)py / (HEIGHT - 1));
}

int mandelbrot(double real, double imag, int max_iter) {
    double zr = 0.0, zi = 0.0;
    int iter = 0;

    while (zr * zr + zi * zi <= 4.0 && iter < max_iter) {
        double temp = zr * zr - zi * zi + real;
        zi = 2.0 * zr * zi + imag;
        zr = temp;
        iter++;
    }
    
    return iter;
}

void thread_work(int start, int end) {
    FILE *image = fopen("mandelbrot.ppm", "a");
    if (!image) {
        perror("Failed to open file");
        return;
    }

    for (int py = start; py < end; py++) {
        for (int px = 0; px < WIDTH; px++) {
            double real = map_pixel_to_real(px);
            double imag = map_pixel_to_imaginary(py);

            int iter = mandelbrot(real, imag, MAX_ITER);

            int color = (255 * iter) / MAX_ITER;
            fprintf(image, "%d %d %d ", color, color, color);
        }
        fprintf(image, "\n");
    }

    if (end == HEIGHT) {
        fprintf(image, "P3\n%d %d\n255\n", WIDTH, HEIGHT);
    }


    fclose(image);
}

void generate_mandelbrot() {
    pthread_t threads[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++) {
        int start = i * HEIGHT / NUM_THREADS;
        int end = (i + 1) * HEIGHT / NUM_THREADS;
        
        pthread_create(&threads[i], NULL, (void *(*)(void *))thread_work, (void *)(&start));
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

int main() {
    clock_t start, end;
    start = clock();
    generate_mandelbrot();
    end = clock();
    printf("Finished in %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);
    return 0;
}
