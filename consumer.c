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
    printf("hello world from the consumer\n");
    printf("creating the /wrote_data semaphore and waiting on it\n");
    sem_t* wrote_data = sem_open("/wrote_data", O_CREAT, S_IRUSR | S_IWUSR);
    sem_wait(wrote_data);
    printf("reading from the \"/mitchmem\" shared memory object\n");
    int shared = shm_open("/mitchmem", O_RDWR, S_IRUSR | S_IWUSR);
    if (shared == -1) {
        printf("failed to open shared memory object\n");
        printf("error: %s\n", strerror(errno));
        return -1;
    }
    printf("mapping shared memory object to a 16 character string\n");
    char* mapped
        = mmap(NULL, 16, PROT_READ | PROT_WRITE, MAP_SHARED, shared, 0);
    printf("the following string is in the shared memory: %s\n", mapped);
    printf("unlinking shared memory\n");
    shm_unlink("/mitchmem");
    return 0;
}