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
const char* DATABLOCK_START = "<<<<<<<<";
const char* DATABLOCK_END = ">>>>>>>>";
const int DATABLOCK_PADDING = 8;
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

typedef struct data_block {
    off_t offset;
    bool isFree;
    ssize_t size;
} Datablock;


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

void sizeToString(ssize_t size, char *str) {
    ssize_t lastSize = size;
    char prefixArr[] = {'B', 'K', 'M', 'G'};
    int index = 0;
    size = size / 1024;
    while (size > 0) {
        lastSize = size;
        index++;
        size = size / 1024;
        if (prefixArr[index] == 'G') {
            break;
        }
    }
    sprintf(str, "%zd%c", lastSize, prefixArr[index]);
}

Catalog getCatalogFromFile(char *filename) {
    ssize_t readBytes;
    Catalog catalog;
    int fdIn = open(filename, O_RDWR);
    if (fdIn < 0) {
        printf("Error opening input file: %s\n", strerror(errno));
        return catalog;
    }
    readBytes = read(fdIn, &catalog, sizeof(Catalog));
    if (readBytes < 0) {
        close(fdIn);
        printf("Error reading file\n");
        return catalog;
    }
    return catalog;
}

int listFiles(char *filename) {
    Catalog catalog = getCatalogFromFile(filename);
    // todo handle error
//    if (catalog == NU) {
//        return -1;
//    }
    for (int i = 0; i < 100; i++) {
        FatItem item = catalog.fat[i];
        if (!item.isEmpty) {
            char itemSize[1024];
            sizeToString(item.size, itemSize);
            //CREDIT print t_time: https://www.tutorialspoint.com/c_standard_library/c_function_ctime.htm
            printf("%s\t%s\t%o\t%s\n", item.filename, itemSize, item.protection, ctime(&item.insertionDataStamp));
        }
    }


};

//todo check if delete
off_t fsize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    fprintf(stderr, "Cannot determine size of %s: %s\n",
            filename, strerror(errno));

    return -1;
}

int sortDatablocksByOffset(const void *a, const void *b) {
    Datablock datablock1;
    Datablock datablock2;
    datablock1 = *(Datablock *) a;
    datablock2 = *(Datablock *) b;
    if (datablock1.isFree) {
        return 1;
    }
    if (datablock2.isFree) {
        return -1;
    }
    int retVal = datablock1.offset - datablock2.offset < 0 ? -1 : 1;
    return (retVal);
}

int sortFreeDatablocksBySize(const void *a, const void *b) {
    Datablock datablock1;
    Datablock datablock2;
    datablock1 = *(Datablock *) a;
    datablock2 = *(Datablock *) b;
    if (!datablock1.isFree) {
        return 1;
    }
    if (!datablock2.isFree) {
        return -1;
    }
    int retVal = datablock1.size - datablock2.size > 0 ? -1 : 1;
    return (retVal);
}

Datablock *getSortedFreeDatablocks(Catalog catalog) {
    off_t currentOffset = sizeof(catalog);
    Datablock *res = (Datablock *) malloc(sizeof(Datablock) * 300);
//    Datablock res[300];
    Datablock occupiedDatablocks[300];
    int index = 0;
    int i;
    // filling occupied datablocks array
    for (i = 0; i < 100; i++) {
        if (!catalog.fat[i].isEmpty) {
            Datablock datablock;
            datablock.size = catalog.fat[i].dataBlock1Size;
            datablock.offset = catalog.fat[i].dataBlock1Offset;
            datablock.isFree = false;
            occupiedDatablocks[index] = datablock;
            index++;
            if (catalog.fat[i].dataBlock2Size > 0) {
                Datablock datablock2;
                datablock2.size = catalog.fat[i].dataBlock2Size;
                datablock2.isFree = false;
                datablock2.offset = catalog.fat[i].dataBlock2Offset;
                occupiedDatablocks[index] = datablock2;
                index++;
            }
            if (catalog.fat[i].dataBlock3Size > 0) {
                Datablock datablock3;
                datablock3.size = catalog.fat[i].dataBlock3Size;
                datablock3.isFree = false;
                datablock3.offset = catalog.fat[i].dataBlock3Offset;
                occupiedDatablocks[index] = datablock3;
                index++;
            }
        }
    }
    for (i = index; i < 300; i++) {
        Datablock datablock;
        datablock.isFree = true;
        occupiedDatablocks[i] = datablock;
    }

    qsort(occupiedDatablocks, 300, sizeof(Datablock), sortDatablocksByOffset);
    //occupied datablocks array should be sorted by offset (all the free blocks should be last)
    index = 0;
    off_t delta;
    for (i = 0; i < 300; i++) {
        if (occupiedDatablocks[i].isFree) {
            break;
        }
        Datablock currDatablock = occupiedDatablocks[i];
        delta = currDatablock.offset - currentOffset;
        if (delta > 0) {
            Datablock freeDatablock;
            freeDatablock.offset = currentOffset + 1;
            freeDatablock.size = delta;
            freeDatablock.isFree = true;
            res[index] = freeDatablock;
            index++;
        }
        currentOffset = currDatablock.offset + currDatablock.size;
    }
    delta = catalog.metadata.fileSize - currentOffset;
    if (delta > 0) {
        Datablock freeDatablock;
        freeDatablock.offset = currentOffset + 1;
        freeDatablock.size = delta;
        freeDatablock.isFree = true;
        res[index] = freeDatablock;
        index++;
    }
    for (i = index; i < 300; i++) {
        Datablock datablock;
        datablock.isFree = false;
        res[i] = datablock;
    }
    qsort(res, 300, sizeof(Datablock), sortFreeDatablocksBySize);
    return res;
}

int writeDatablockToFile(int fd, Datablock datablock, char* data){
    //fd should be file descriptor that is able to overwrite
    // data size should be datablock size
    char buffer[BUFFER];
    int bufferIndex = 0;
    ssize_t bytesWritten;
    ssize_t datablockSize = datablock.size - (2 * DATABLOCK_PADDING);
    if (datablockSize <1) {
        printf("Datablock size is too small\n");
        return -1;
    }
    lseek(fd, datablock.offset, SEEK_SET);
    bytesWritten = write(fd, DATABLOCK_START, DATABLOCK_PADDING);
    if (bytesWritten < 0) {
        printf("Error when writing to file\n");
        return -1;
    }
    for (size_t i = 0; i< datablockSize; i++) {
        buffer[bufferIndex] = data[i];
        bufferIndex ++;
        if (bufferIndex == BUFFER) {
            bufferIndex = 0;
            write(fd, buffer, BUFFER);
            if (bytesWritten < 0) {
                printf("Error when writing to file\n");
                return -1;
            }
        }
    }
    if (bufferIndex > 0) {
        write(fd, buffer, (size_t)bufferIndex);
        if (bytesWritten < 0) {
            printf("Error when writing to file\n");
            return -1;
        }
    }
    write(fd, DATABLOCK_END, DATABLOCK_PADDING);
    if (bytesWritten < 0) {
        printf("Error when writing to file\n");
        return -1;
    }
    return 0;
}

int insertFile(char *vaultName, char *filePath) {


};

int main() {
    init("./hello.txt", "1M");
    Catalog catalog1;
    catalog1 = getCatalogFromFile("./hello.txt");
    Datablock *datablock = getSortedFreeDatablocks(catalog1);

}