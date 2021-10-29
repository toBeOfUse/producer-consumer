#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main()
{
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

    printf("setting /wrote_data semaphore to tell the consumer to read fro the "
           "object\n");
    sem_t* wrote_data = sem_open("/wrote_data", O_CREAT, S_IRUSR | S_IWUSR);
    sem_post(wrote_data);
    return 0;
}