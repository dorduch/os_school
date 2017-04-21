#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>
#include <memory.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h> // for open flags
#include <time.h> // for time measurement
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


const int TEXT_LENGTH = 257;
const int BUFFER = 4 * 1024;
#define _FILE_OFFSET_BITS 64


typedef struct metadata {
    ssize_t fileSize;
    time_t creationTime;
    time_t lastModified;
    short numOfFiles;
} Metadata;

typedef struct fat_item {
    char filename[TEXT_LENGTH];
    ssize_t size;
    mode_t protection;
    time_t insertionDataStamp;
    off_t dataBlock1Offset;
    ssize_t dataBlock1Size;
    off_t dataBlock2Offset;
    ssize_t dataBlock2Size;
    off_t dataBlock3Offset;
    ssize_t dataBlock3Size;
    bool isEmpty;
} FatItem;


typedef struct catalog {
    Metadata metadata;
    FatItem fat[100];
} Catalog;


int init(char *filename, char *sizeString) {

    //CREDIT HW1
    char dataAmountSizeSuffix = sizeString[(int) strlen(sizeString) - 1];
    int dataAmountSize = atoi(sizeString);
    size_t dataAmountSizeBytes;

    if (dataAmountSizeSuffix == 'b' || dataAmountSizeSuffix == 'B') {
        dataAmountSizeBytes = (size_t) dataAmountSize;
    } else if (dataAmountSizeSuffix == 'k' || dataAmountSizeSuffix == 'K') {
        dataAmountSizeBytes = (size_t) dataAmountSize * 1024;
    } else if (dataAmountSizeSuffix == 'm' || dataAmountSizeSuffix == 'M') {
        dataAmountSizeBytes = (size_t) dataAmountSize * 1024 * 1204;
    } else if (dataAmountSizeSuffix == 'g' || dataAmountSizeSuffix == 'G') {
        dataAmountSizeBytes = (size_t) dataAmountSize * 1204 * 1024 * 1204;
    } else {
        printf("Not valid data amount:%c\n", dataAmountSizeSuffix);
        return -1;
    }

    struct timeval timestamp;

    Metadata metadata;
    gettimeofday(&timestamp, NULL);
    metadata.creationTime = timestamp.tv_sec;
    metadata.fileSize = dataAmountSizeBytes;
    metadata.lastModified = timestamp.tv_sec;
    metadata.numOfFiles = 0;

    Catalog catalog;
    catalog.metadata = metadata;
    for (int i = 0; i < 100; i++) {
        FatItem fatItem;
        fatItem.isEmpty = true;
        catalog.fat[i] = fatItem;
    }

    if (sizeof(catalog) >= dataAmountSizeBytes) {
        printf("file size isn't big enough\n");
        return -1;
    } else {
        int fdOut = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        ssize_t bytesWritten = write(fdOut, &catalog, sizeof(catalog));
        ssize_t totalBytesWritten;
        if (bytesWritten < 0) {
            close(fdOut);
            printf("Error when writing to file\n");
            return -1;
        }
        totalBytesWritten = bytesWritten;
        char junkArray[BUFFER];
        while (totalBytesWritten < dataAmountSizeBytes) {
            size_t delta = dataAmountSizeBytes - totalBytesWritten;
            size_t bytesToWrite = delta > BUFFER ? BUFFER : delta;
            bytesWritten = write(fdOut, &junkArray, bytesToWrite);
            if (bytesWritten < 0) {
                close(fdOut);
                printf("Error when writing to file\n");
                return -1;
            }
            totalBytesWritten += bytesWritten;
        }
        close(fdOut);
    }
    return 0;
};

void sizeToString(ssize_t size, char* str){
    ssize_t lastSize = size;
    char prefixArr[] = {'B', 'K', 'M', 'G'};
    int index = 0;
    size = size / 1024;
    while(size > 0) {
        lastSize = size;
        index ++;
        size = size / 1024;
        if (prefixArr[index] == 'G') {
            break;
        }
    }
    sprintf(str, "%zd%c", lastSize, prefixArr[index]);
}

int getCatalogFromFile(char* filename, Catalog* catalog){
    ssize_t readBytes;
    int fdIn = open(filename, O_RDWR);
    if (fdIn < 0) {
        printf("Error opening input file: %s\n", strerror(errno));
        return -1;
    }
    readBytes = read(fdIn, &catalog, sizeof(catalog));
    if (readBytes < 0) {
        close(fdIn);
        printf("Error reading file\n");
        return -1;
    }
}

int listFiles(char *filename) {
    Catalog catalog;
    if (getCatalogFromFile(filename, &catalog) == -1) {
        return -1;
    }
    for (int i = 0; i<100; i++) {
        FatItem item = catalog.fat[i];
        if (!item.isEmpty) {
            char itemSize[1024];
            sizeToString(item.size, itemSize);
            //CREDIT print t_time: https://www.tutorialspoint.com/c_standard_library/c_function_ctime.htm
            printf("%s\t%s\t%o\t%s\n", item.filename, itemSize, item.protection, ctime(&item.insertionDataStamp));
        }
    }


};

int main() {
    init("./hello.txt", "34450B");

}