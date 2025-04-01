#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define WIDTH 1920
#define HEIGHT 1080
#define MAX_ITER 1000
#define NUM_THREADS 24

const double X_MIN = -2.0;
const double X_MAX = 1.0;
const double Y_MIN = -1.5;
const double Y_MAX = 1.5;

unsigned char *image_buffer;

typedef struct {
    int thread_id;
    int start_row;
    int end_row;
} ThreadData;

double map_to_real(int x) {
    return X_MIN + (X_MAX - X_MIN) * ((double)x / (WIDTH - 1));
}

double map_to_imaginary(int y) {
    return Y_MIN + (Y_MAX - Y_MIN) * ((double)y / (HEIGHT - 1));
}

int calculate_mandelbrot(double real, double imag) {
    double zr = 0.0;
    double zi = 0.0;
    double zr_new, zi_new;
    int i;
    
    for (i = 0; i < MAX_ITER; i++) {
        if (zr*zr + zi*zi > 4.0) 
            break;
            
        // z = z² + c
        zr_new = zr*zr - zi*zi + real;
        zi_new = 2.0*zr*zi + imag;
        
        zr = zr_new;
        zi = zi_new;
    }
    
    return i;
}

void map_to_color(int iterations, unsigned char *r, unsigned char *g, unsigned char *b) {
    if (iterations == MAX_ITER) {
        // Points in the set are black
        *r = 0;
        *g = 0;
        *b = 0;
    } else {
        double t = (double)iterations / MAX_ITER;
        
        *r = *g = *b = (unsigned char)(255 * t);
        
        /* Uncomment for a more colorful version:
        *r = (unsigned char)(9 * (1 - t) * t * t * t * 255);
        *g = (unsigned char)(15 * (1 - t) * (1 - t) * t * t * 255);
        *b = (unsigned char)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
        */
    }
}

void* render_section(void *data) {
    ThreadData *thread_data = (ThreadData*)data;
    int start_row = thread_data->start_row;
    int end_row = thread_data->end_row;
    
    for (int y = start_row; y < end_row; y++) {
        for (int x = 0; x < WIDTH; x++) {
            double real = map_to_real(x);
            double imag = map_to_imaginary(y);
            
            int iterations = calculate_mandelbrot(real, imag);
            
            unsigned char r, g, b;
            map_to_color(iterations, &r, &g, &b);
            
            int pos = 3 * (y * WIDTH + x);
            
            image_buffer[pos] = r;
            image_buffer[pos + 1] = g;
            image_buffer[pos + 2] = b;
        }
    }
    
    return NULL;
}

void save_image(const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Error opening output file");
        return;
    }
    
    fprintf(file, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
    
    fwrite(image_buffer, sizeof(unsigned char), WIDTH * HEIGHT * 3, file);
    
    fclose(file);
    printf("Image saved to %s\n", filename);
}

int main() {
    clock_t start_time, end_time;
    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    
    printf("Generating %dx%d Mandelbrot set image using %d threads...\n", WIDTH, HEIGHT, NUM_THREADS);
    
    image_buffer = (unsigned char*)malloc(WIDTH * HEIGHT * 3 * sizeof(unsigned char));
    if (!image_buffer) {
        perror("Memory allocation failed");
        return 1;
    }
    
    // Start timing
    start_time = clock();
    
    // Create and launch threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].start_row = i * HEIGHT / NUM_THREADS;
        thread_data[i].end_row = (i + 1) * HEIGHT / NUM_THREADS;
        
        if (pthread_create(&threads[i], NULL, render_section, &thread_data[i]) != 0) {
            perror("Thread creation failed");
            free(image_buffer);
            return 1;
        }
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    end_time = clock();
    double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    save_image("mandelbrot.ppm");
    
    // Clean up
    free(image_buffer);
    
    printf("Finished in %.2f seconds\n", elapsed_time);
    return 0;
}
