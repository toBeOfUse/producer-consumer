#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

// define image data type and utility functions
typedef struct image {
    int width;
    int height;
    int num_channels;
    unsigned char* data; // data returned from stbi_load
} image;

int load_image(const char* image_path, image* result)
{
    result->data = stbi_load(
        image_path, &result->width, &result->height, &result->num_channels, 0);
    if (result->data == NULL) {
        printf("could not load image from %s\n", image_path);
        return -1;
    }
    return 0;
}

void free_image(image old) { stbi_image_free(old.data); }

// define rect data type and utility functions
typedef struct rect {
    int x;
    int y;
    int width;
    int height;
} rect;

int is_point_in_rect(rect r, int x, int y)
{
    return x - r.x > 0 && x - r.x < r.width && y - r.y > 0
        && y - r.y < r.height;
}

rect get_random_rect_within_rect(
    rect r, int new_rect_width, int new_rect_height)
{
    int maxX = r.width - new_rect_width;
    int maxY = r.height - new_rect_height;
    srand(time(NULL));
    rect result;
    result.x = rand() % (maxX + 1);
    result.y = rand() % (maxY + 1);
    result.width = new_rect_width;
    result.height = new_rect_height;
    return result;
}

// define worker functions

/*
 * Copies the non-completely-transparent contents of `layer` into the data of
 * `dest`, starting from the top left corner given by top_left_x, top_left_y,
 * scaling the alpha component of each copied pixel so that the greatest value
 * it can have is that given by the paramter `alpha`.
 */
void layer_image_with_alpha(image layer, image* dest, int top_left_x,
    int top_left_y, unsigned char alpha)
{
    for (int y = 0; y < layer.height; y++) {
        for (int x = 0; x < layer.width; x++) {
            unsigned char* source = &layer.data[(y * layer.width + x) * 4];
            if (source[3] == 0) {
                continue;
            }
            int destx = x + top_left_x;
            int desty = y + top_left_y;
            unsigned char* dest_pos
                = &dest->data[(desty * dest->width + destx) * 4];
            for (int i = 0; i < 3; i++) {
                dest_pos[i] = source[i];
            }
            dest_pos[3] = alpha * (source[3] / 255.0);
        }
    }
}

int main()
{
    // hello world example
    printf("hello world from the producer\n");
    shm_unlink("/mitchmem"); // just in case it exists from a previous version
                             // of this program that crashed
    printf("creating shared memory object with name \"/mitchmem\"\n");
    int shared
        = shm_open("/mitchmem", O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (shared == -1) {
        printf("failed to create shared memory object\n");
        printf("error: %s\n", strerror(errno));
        return -1;
    }

    printf("setting size of shared memory object with name \"/mitchmem\" to 16 "
           "bytes\n");
    ftruncate(shared, 16);

    printf("mapping shared memory to a 16 character string\n");
    char* mapped
        = mmap(NULL, 16, PROT_READ | PROT_WRITE, MAP_SHARED, shared, 0);

    printf("writing text \"Hello World\" into shared memory\n");
    strcpy(mapped, "Hello World");

    printf(
        "setting /wrote_data semaphore to tell the consumer to read from the "
        "object\n");
    sem_t* wrote_data = sem_open("/wrote_data", O_CREAT, S_IRUSR | S_IWUSR);
    sem_post(wrote_data);

    // image sparkle example

    image base;
    if (load_image("testimages/base.jpg", &base) == -1) {
        printf("could not load base image!\n");
        return -1;
    }

    image sparkle;
    if (load_image("testimages/sparkle.png", &sparkle) == -1) {
        printf("could not load sparkle image!\n");
        return -1;
    }
    if (sparkle.num_channels != 4) {
        printf("expected sparkle image with RGBA data (so like, a PNG with a "
               "transparent background)\n");
        return -1;
    }

    // basic test: add one sparkle

    printf("adding sparkle\n");
    image dest = { base.width, base.height, 4,
        calloc(base.width * base.height * 4, 1) };
    layer_image_with_alpha(sparkle, &dest, 50, 50, 100);
    printf("writing new file\n");
    if (!stbi_write_png("./result.png", dest.width, dest.height,
            dest.num_channels, dest.data, dest.width * dest.num_channels)) {
        printf("could not write file\n");
    }

    free_image(base);
    free_image(sparkle);

    return 0;
}