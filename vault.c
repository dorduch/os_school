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
#include <libgen.h>
#include <ctype.h>


const int TEXT_LENGTH = 257;
const char *DATABLOCK_START = "<<<<<<<<";
const char *DATABLOCK_END = ">>>>>>>>";
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
    int fatIndex;
    int blockNum;
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

int getCatalogFromFile(int fdIn, Catalog *catalog) {
    ssize_t readBytes;
    Catalog res;
    readBytes = read(fdIn, &res, sizeof(Catalog));
    if (readBytes < 0) {
        printf("Error reading file\n");
        return -1;
    }
    *catalog = res;
    return 0;
}

int writeCatalogToVault(int vaultFd, Catalog catalog) {
    ssize_t BytesWritten;
    lseek(vaultFd, 0, SEEK_SET);
    BytesWritten = write(vaultFd, &catalog, sizeof(Catalog));
    if (BytesWritten < 0) {
        printf("Error reading file\n");
        return -1;
    }
    return 0;
}

int listFiles(char *filename) {
    Catalog catalog;
    int fdIn = open(filename, O_RDONLY);
    if (fdIn < 0) {
        printf("Error reading file\n");
        return -1;
    }
    if (getCatalogFromFile(fdIn, &catalog) == -1) {
        printf("Error when opening vault\n");
        close(fdIn);
        return -1;
    };
    for (int i = 0; i < 100; i++) {
        FatItem item = catalog.fat[i];
        if (!item.isEmpty) {
            char itemSize[1024];
            sizeToString(item.size, itemSize);
            //CREDIT print t_time: https://www.tutorialspoint.com/c_standard_library/c_function_ctime.htm
            printf("%s\t%s\t%o\t%s", item.filename, itemSize, item.protection, ctime(&item.insertionDataStamp));
        }
    }

    close(fdIn);

    return 0;
};

//CREDIT check file size: http://www.linuxquestions.org/questions/programming-9/how-to-get-size-of-file-in-c-183360/
off_t getFileSize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    fprintf(stderr, "Cannot determine size of %s: %s\n",
            filename, strerror(errno));

    return -1;
}

//CREDIT get file permissions: http://stackoverflow.com/questions/20238042/is-there-a-c-function-to-get-permissions-of-a-file
mode_t getFilePermissions(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_mode;

    fprintf(stderr, "Cannot determine permissions of %s: %s\n",
            filename, strerror(errno));

    return NULL;
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

Datablock *getDatablocksByOffset(Catalog catalog) {
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
            datablock.fatIndex = i;
            datablock.blockNum = 1;
            occupiedDatablocks[index] = datablock;
            index++;
            if (catalog.fat[i].dataBlock2Size > 0) {
                Datablock datablock2;
                datablock2.size = catalog.fat[i].dataBlock2Size;
                datablock2.isFree = false;
                datablock2.offset = catalog.fat[i].dataBlock2Offset;
                datablock.fatIndex = i;
                datablock.blockNum = 2;
                occupiedDatablocks[index] = datablock2;
                index++;
            }
            if (catalog.fat[i].dataBlock3Size > 0) {
                Datablock datablock3;
                datablock3.size = catalog.fat[i].dataBlock3Size;
                datablock3.isFree = false;
                datablock3.offset = catalog.fat[i].dataBlock3Offset;
                datablock.fatIndex = i;
                datablock.blockNum = 3;
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
    return occupiedDatablocks;
}

Datablock *getSortedFreeDatablocks(Catalog catalog) {
    off_t currentOffset = sizeof(catalog);
    Datablock *res = (Datablock *) malloc(sizeof(Datablock) * 300);
//    Datablock res[300];
//    Datablock occupiedDatablocks[300];
    int index;
    int i;
    Datablock *occupiedDatablocks = getDatablocksByOffset(catalog);

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

int writeDatablocksToFile(int outputFd, Datablock *datablocksArr, int fileFd, int numOfBlocks, size_t fileSize) {
    //fd should be file descriptor that is able to overwrite
    char inBuffer[BUFFER];
    for (int j = 0; j < numOfBlocks; j++) {
        ssize_t totalBytesRead = 0;
        ssize_t bytesWritten;
        ssize_t delta;
        ssize_t bytesRead;
        ssize_t datablockSize = datablocksArr[j].size - (2 * DATABLOCK_PADDING);
        lseek(outputFd, datablocksArr[j].offset, SEEK_SET);
        bytesWritten = write(outputFd, DATABLOCK_START, DATABLOCK_PADDING);
        if (bytesWritten < 0) {
            printf("Error when writing to file\n");
            deleteBlockFromFile(datablocksArr[j].size, datablocksArr[j].offset, outputFd);
            return -1;
        }

        bytesRead = read(fileFd, inBuffer, BUFFER);
        if (bytesRead < 0) {
            printf("Error when reading from file\n");
            deleteBlockFromFile(datablocksArr[j].size, datablocksArr[j].offset, outputFd);
            return -1;
        }
        totalBytesRead += bytesRead;
        while (totalBytesRead < datablockSize) {
            bytesWritten = write(outputFd, inBuffer, (size_t) bytesRead);
            if (bytesWritten < 0) {
                printf("Error when writing to file\n");
                deleteBlockFromFile(datablocksArr[j].size, datablocksArr[j].offset, outputFd);
                return -1;
            }
            bytesRead = read(fileFd, inBuffer, BUFFER);
            if (bytesRead < 0) {
                printf("Error when reading from file\n");
                deleteBlockFromFile(datablocksArr[j].size, datablocksArr[j].offset, outputFd);
                return -1;
            }
            if (bytesRead == 0) {
                break;
            }
            totalBytesRead += bytesRead;
        }
        delta = fileSize - totalBytesRead;
        if (delta > 0) {
            bytesWritten = write(outputFd, inBuffer, (size_t) delta);
            if (bytesWritten < 0) {
                printf("Error when writing to file\n");
                deleteBlockFromFile(datablocksArr[j].size, datablocksArr[j].offset, outputFd);
                return -1;
            }
        }
        write(outputFd, DATABLOCK_END, DATABLOCK_PADDING);
        if (bytesWritten < 0) {
            printf("Error when writing to file\n");
            deleteBlockFromFile(datablocksArr[j].size, datablocksArr[j].offset, outputFd);
            return -1;
        }
        lseek(fileFd, datablockSize + 1, SEEK_SET);
    }

    return 0;
}

int insertFile(char *vaultName, char *filePath) {
    int i;
    struct timeval timestamp;
    int vaultFd = open(vaultName, O_RDWR);
    if (vaultFd < 0) {
        printf("Error opening input file: %s\n", strerror(errno));
        return -1;
    }

    int fileFd = open(filePath, O_RDONLY);
    if (fileFd < 0) {
        printf("Error opening input file: %s\n", strerror(errno));
        close(vaultFd);
        return -1;
    }

    Catalog catalog;
    if (getCatalogFromFile(vaultFd, &catalog) == -1) {
        printf("Error opening input file\n");
        close(fileFd);
        close(vaultFd);
        return -1;
    };

    int freeFatSlotIndex = -1;
    for (i = 0; i < 100; i++) {
        if (catalog.fat[i].isEmpty) {
            freeFatSlotIndex = i;
            break;
        }
    }
    if (freeFatSlotIndex == -1) {
        printf("File capacity exceeded\n");
        close(fileFd);
        close(vaultFd);
        return 0;
    }

    off_t fileSize = getFileSize(filePath);
    mode_t filePermissions = getFilePermissions(filePath);
    if (fileSize == 0) {
        printf("File is empty\n");
        close(fileFd);
        close(vaultFd);
        return 0;
    }

    if (filePermissions == NULL) {
        close(fileFd);
        close(vaultFd);
        return -1;
    }
    Datablock *freeDatablocks = getSortedFreeDatablocks(catalog);
    int numOfBlocks = 0;
    if (freeDatablocks[0].isFree && fileSize <= freeDatablocks[0].size + 2 * DATABLOCK_PADDING) {
        numOfBlocks = 1;
    } else if (freeDatablocks[0].isFree && freeDatablocks[1].isFree &&
               fileSize <= (freeDatablocks[0].size + freeDatablocks[1].size + 4 * DATABLOCK_PADDING)) {
        numOfBlocks = 2;
    } else if (freeDatablocks[0].isFree && freeDatablocks[1].isFree && freeDatablocks[2].isFree &&
               fileSize <=
               (freeDatablocks[0].size + freeDatablocks[1].size + freeDatablocks[2].size + 6 * DATABLOCK_PADDING)) {
        numOfBlocks = 3;
    } else {
        printf("Not enough free space to insert file\n");
        close(fileFd);
        close(vaultFd);
        return 0;
    }
    gettimeofday(&timestamp, NULL);
    catalog.fat[freeFatSlotIndex].isEmpty = false;
    catalog.fat[freeFatSlotIndex].size = fileSize;
    strcpy(catalog.fat[freeFatSlotIndex].filename, basename(filePath));
    catalog.fat[freeFatSlotIndex].insertionDataStamp = timestamp.tv_sec;
    catalog.fat[freeFatSlotIndex].protection = filePermissions;
    catalog.metadata.numOfFiles++;
    catalog.metadata.lastModified = timestamp.tv_sec;

    if (numOfBlocks == 1) {
        catalog.fat[freeFatSlotIndex].dataBlock1Size = fileSize + 2 * DATABLOCK_PADDING;
        catalog.fat[freeFatSlotIndex].dataBlock2Size = 0;
        catalog.fat[freeFatSlotIndex].dataBlock3Size = 0;
        catalog.fat[freeFatSlotIndex].dataBlock1Offset = freeDatablocks[0].offset;
        catalog.fat[freeFatSlotIndex].dataBlock2Offset = 0;
        catalog.fat[freeFatSlotIndex].dataBlock3Offset = 0;
    } else if (numOfBlocks == 2) {
        catalog.fat[freeFatSlotIndex].dataBlock1Size = freeDatablocks[0].size;
        catalog.fat[freeFatSlotIndex].dataBlock2Size = (fileSize + 2 * DATABLOCK_PADDING) - freeDatablocks[0].size;
        catalog.fat[freeFatSlotIndex].dataBlock3Size = 0;
        catalog.fat[freeFatSlotIndex].dataBlock1Offset = freeDatablocks[0].offset;
        catalog.fat[freeFatSlotIndex].dataBlock2Offset = freeDatablocks[1].offset;
        catalog.fat[freeFatSlotIndex].dataBlock3Offset = 0;
    } else {
        catalog.fat[freeFatSlotIndex].dataBlock1Size = freeDatablocks[0].size;
        catalog.fat[freeFatSlotIndex].dataBlock2Size = freeDatablocks[1].size;
        catalog.fat[freeFatSlotIndex].dataBlock3Size =
                (fileSize + 2 * DATABLOCK_PADDING) - freeDatablocks[0].size - freeDatablocks[1].size;
        catalog.fat[freeFatSlotIndex].dataBlock1Offset = freeDatablocks[0].offset;
        catalog.fat[freeFatSlotIndex].dataBlock2Offset = freeDatablocks[1].offset;
        catalog.fat[freeFatSlotIndex].dataBlock3Offset = freeDatablocks[2].offset;
    }


    if (writeDatablocksToFile(vaultFd, freeDatablocks, fileFd, numOfBlocks, (size_t) fileSize) == -1) {
        close(fileFd);
        close(vaultFd);
        return -1;
    };

    if (writeCatalogToVault(vaultFd, catalog) == -1) {
        close(fileFd);
        close(vaultFd);
        return -1;
    }

    close(fileFd);
    close(vaultFd);

    return 0;

};

int getFilenameFatIndex(const char *filename, Catalog *catalog) {
    int fatIndex = -1;

    for (int i = 0; i < 100; i++) {
        if (strcmp((*catalog).fat[i].filename, filename) == 0) {
            fatIndex = i;
            break;
        }
    }
    return fatIndex;
}

int deleteBlockFromFile(ssize_t size, off_t offset, int fd) {
    char writeBuffer[8];
    ssize_t bytesWritten;
    lseek(fd, offset, SEEK_SET);
    bytesWritten = write(fd, writeBuffer, 8);
    if (bytesWritten < 0) {
        printf("Can't delete block from vault\n");
        return -1;
    }
    lseek(fd, size - 16, SEEK_CUR);
    bytesWritten = write(fd, writeBuffer, 8);
    if (bytesWritten < 0) {
        printf("Can't delete block from vault\n");
        return -1;
    }
    return 0;
};

int deleteFile(char *vaultName, char *filename) {
    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);
    int vaultFd = open(vaultName, O_RDWR);
    if (vaultFd < 0) {
        printf("Error opening input file: %s\n", strerror(errno));
        return -1;
    }
    Catalog catalog;
    if (getCatalogFromFile(vaultFd, &catalog) == -1) {
        printf("Error opening input file\n");
        return -1;
    };
    int fatIndex = getFilenameFatIndex(filename, &catalog);

    if (fatIndex == -1) {
        printf("File not exists in vault\n");
        close(vaultFd);
        return -1;
    }

    catalog.fat[fatIndex].isEmpty = true;
    if (catalog.fat[fatIndex].dataBlock1Size > 0) {
        if (deleteBlockFromFile(catalog.fat[fatIndex].dataBlock1Size, catalog.fat[fatIndex].dataBlock1Offset,
                                vaultFd) == -1) {
            close(vaultFd);
            return -1;
        };
    }
    if (catalog.fat[fatIndex].dataBlock2Size > 0) {
        if (deleteBlockFromFile(catalog.fat[fatIndex].dataBlock2Size, catalog.fat[fatIndex].dataBlock2Offset,
                                vaultFd) == -1) {
            close(vaultFd);
            return -1;
        };
    }
    if (catalog.fat[fatIndex].dataBlock3Size > 0) {
        if (deleteBlockFromFile(catalog.fat[fatIndex].dataBlock3Size, catalog.fat[fatIndex].dataBlock3Offset,
                                vaultFd) == -1) {
            close(vaultFd);
            return -1;
        };
    }

    catalog.metadata.numOfFiles--;
    catalog.metadata.lastModified = timestamp.tv_sec;
    if (writeCatalogToVault(vaultFd, catalog) == -1) {
        close(vaultFd);
        return -1;
    };
    close(vaultFd);
    return 0;
}

int fetchFile(char *vaultName, char *filename) {
    int vaultFd = open(vaultName, O_RDONLY);
    int outFd;
    ssize_t bytesRead;
    ssize_t bytesWritten;
    ssize_t totalBytesRead = 0;
    ssize_t delta;
    ssize_t toRead;
    char outBuffer[BUFFER];
    if (vaultFd < 0) {
        printf("Error opening input file: %s\n", strerror(errno));
        return -1;
    }
    Catalog catalog;
    if (getCatalogFromFile(vaultFd, &catalog) == -1) {
        printf("Error opening input file\n");
        return -1;
    };
    int fatIndex = getFilenameFatIndex(filename, &catalog);

    if (fatIndex == -1) {
        printf("File not exists in vault\n");
        close(vaultFd);
        return -1;
    }

    int result = access("./", W_OK);
    if (result != 0) {
        printf("User doesn't have permissions to write in the directory\n");
        close(vaultFd);
        return -1;
    }

    //CREDIT create file: http://pubs.opengroup.org/onlinepubs/009695399/functions/creat.html
    outFd = creat(filename, catalog.fat[fatIndex].protection);
    if (outFd < 0) {
        printf("File could not be created\n");
        close(vaultFd);
        return -1;
    }

    FatItem fatItem = catalog.fat[fatIndex];
    if (fatItem.dataBlock1Size > 0) {
        ssize_t sizeNoPadding = fatItem.dataBlock1Size - 2 * DATABLOCK_PADDING;
        lseek(vaultFd, catalog.fat[fatIndex].dataBlock1Offset + DATABLOCK_PADDING, SEEK_SET);
        toRead = sizeNoPadding - totalBytesRead > BUFFER ? BUFFER : sizeNoPadding - totalBytesRead;
        bytesRead = read(vaultFd, outBuffer, (size_t) toRead);
        if (bytesRead < 0) {
            printf("Can't read from vault\n");
            close(vaultFd);
            close(outFd);
            return -1;
        };
        totalBytesRead += bytesRead;
        while (totalBytesRead <= sizeNoPadding) {
            bytesWritten = write(outFd, outBuffer, (size_t) toRead);
            if (bytesWritten < 0) {
                printf("Can't write to file\n");
                close(vaultFd);
                close(outFd);
                return -1;
            };
            toRead = sizeNoPadding - totalBytesRead > BUFFER ? BUFFER : sizeNoPadding - totalBytesRead;
            if (toRead == 0) {
                break;
            }
            bytesRead = read(vaultFd, outBuffer, (size_t) toRead);
            if (bytesRead < 0) {
                printf("Can't read from vault\n");
                close(vaultFd);
                close(outFd);
                return -1;
            };
            totalBytesRead += bytesRead;
        }
        delta = sizeNoPadding - totalBytesRead;
        if (delta > 0) {
            bytesWritten = write(outFd, outBuffer, (size_t) toRead);
            if (bytesWritten < 0) {
                printf("Can't write to file\n");
                close(vaultFd);
                close(outFd);
                return -1;
            };
        }
    }

    if (fatItem.dataBlock2Size > 0) {
        ssize_t sizeNoPadding = fatItem.dataBlock2Size - 2 * DATABLOCK_PADDING;
        lseek(vaultFd, catalog.fat[fatIndex].dataBlock2Offset + DATABLOCK_PADDING, SEEK_SET);
        toRead = sizeNoPadding - totalBytesRead > BUFFER ? BUFFER : sizeNoPadding - totalBytesRead;
        bytesRead = read(vaultFd, outBuffer, (size_t) toRead);
        if (bytesRead < 0) {
            printf("Can't read from vault\n");
            close(vaultFd);
            close(outFd);
            return -1;
        };
        totalBytesRead += bytesRead;
        while (totalBytesRead <= sizeNoPadding) {
            bytesWritten = write(outFd, outBuffer, (size_t) toRead);
            if (bytesWritten < 0) {
                printf("Can't write to file\n");
                close(vaultFd);
                close(outFd);
                return -1;
            };
            toRead = sizeNoPadding - totalBytesRead > BUFFER ? BUFFER : sizeNoPadding - totalBytesRead;
            if (toRead == 0) {
                break;
            }
            bytesRead = read(vaultFd, outBuffer, (size_t) toRead);
            if (bytesRead < 0) {
                printf("Can't read from vault\n");
                close(vaultFd);
                close(outFd);
                return -1;
            };
            totalBytesRead += bytesRead;
        }
        delta = sizeNoPadding - totalBytesRead;
        if (delta > 0) {
            bytesWritten = write(outFd, outBuffer, (size_t) toRead);
            if (bytesWritten < 0) {
                printf("Can't write to file\n");
                close(vaultFd);
                close(outFd);
                return -1;
            };
        }
    }
    if (fatItem.dataBlock3Size > 0) {
        ssize_t sizeNoPadding = fatItem.dataBlock3Size - 2 * DATABLOCK_PADDING;
        lseek(vaultFd, catalog.fat[fatIndex].dataBlock3Offset + DATABLOCK_PADDING, SEEK_SET);
        toRead = sizeNoPadding - totalBytesRead > BUFFER ? BUFFER : sizeNoPadding - totalBytesRead;
        bytesRead = read(vaultFd, outBuffer, (size_t) toRead);
        if (bytesRead < 0) {
            printf("Can't read from vault\n");
            close(vaultFd);
            close(outFd);
            return -1;
        };
        totalBytesRead += bytesRead;
        while (totalBytesRead <= sizeNoPadding) {
            bytesWritten = write(outFd, outBuffer, (size_t) toRead);
            if (bytesWritten < 0) {
                printf("Can't write to file\n");
                close(vaultFd);
                close(outFd);
                return -1;
            };
            toRead = sizeNoPadding - totalBytesRead > BUFFER ? BUFFER : sizeNoPadding - totalBytesRead;
            if (toRead == 0) {
                break;
            }
            bytesRead = read(vaultFd, outBuffer, (size_t) toRead);
            if (bytesRead < 0) {
                printf("Can't read from vault\n");
                close(vaultFd);
                close(outFd);
                return -1;
            };
            totalBytesRead += bytesRead;
        }
        delta = sizeNoPadding - totalBytesRead;
        if (delta > 0) {
            bytesWritten = write(outFd, outBuffer, (size_t) toRead);
            if (bytesWritten < 0) {
                printf("Can't write to file\n");
                close(vaultFd);
                close(outFd);
                return -1;
            };
        }
    }
    chmod(filename, catalog.fat[fatIndex].protection);
    close(vaultFd);
    close(outFd);
    return 0;
};

int moveDatablockInFile(int fdIn, int fdOut, ssize_t size, off_t oldOffset, off_t newOffset, off_t offsetDelta) {
    size_t totalBytesRead = 0;
    ssize_t bytesRead;
    ssize_t bytesWritten;
    ssize_t delta;
    size_t bytesToRead;
    char buffer[BUFFER];
    char junk[DATABLOCK_PADDING];

    lseek(fdIn, oldOffset, SEEK_SET);
    lseek(fdOut, newOffset, SEEK_SET);
    bytesToRead = size - totalBytesRead > BUFFER ? BUFFER : size - totalBytesRead;
    bytesRead = read(fdIn, buffer, bytesToRead);
    if (bytesRead < 0) {
        return -1;
    }
    totalBytesRead += bytesRead;
    while (totalBytesRead <= size) {
        bytesWritten = write(fdOut, buffer, bytesToRead);
        if (bytesWritten < 0) {
            return -1;
        }
        bytesToRead =
                size - totalBytesRead > BUFFER ? BUFFER : size - totalBytesRead;
        bytesRead = read(fdIn, buffer, bytesToRead);
        if (bytesRead < 0) {
            return -1;
        }
        if (bytesRead == 0) {
            break;
        }
        totalBytesRead += bytesRead;
    }
    delta = size - totalBytesRead;
    if (delta > 0) {
        bytesWritten = write(fdOut, buffer, (size_t) delta);
        if (bytesWritten < 0) {
            return -1;
        }
    }
    // delete arrows
    if (offsetDelta > size) {
        deleteBlockFromFile(size, oldOffset, fdOut);
    } else {
        lseek(fdOut, oldOffset + size - DATABLOCK_PADDING, SEEK_SET);
        bytesWritten = write(fdOut, junk, DATABLOCK_PADDING);
        if (bytesWritten < 0) {
            return -1;
        }
    }
    return 0;
}

int defrag(char *vaultName) {
    off_t offsetDelta;
    int i;

    int fdIn = open(vaultName, O_RDONLY);
    int fdOut = open(vaultName, O_WRONLY);
    Catalog catalog;
    if (fdIn < 0) {
        printf("Error opening input file: %s\n", strerror(errno));
        return -1;
    }
    if (fdOut < 0) {
        printf("Error opening input file: %s\n", strerror(errno));
        close(fdIn);
        return -1;
    }
    if (getCatalogFromFile(fdIn, &catalog) == -1) {
        printf("Error opening input file\n");
        return -1;
    };
    off_t currentOffset = sizeof(catalog) + 1;
    Datablock *occupiedDatablocks = getDatablocksByOffset(catalog);
    for (i = 0; i < 300; i++) {
        if (occupiedDatablocks[i].isFree) {
            break;
        }
        Datablock currDatablock = occupiedDatablocks[i];
        offsetDelta = currDatablock.offset - currentOffset;
        if (offsetDelta > 1) {
            if (moveDatablockInFile(fdIn, fdOut, currDatablock.size, currDatablock.offset, currentOffset,
                                    offsetDelta) == -1) {
                printf("Error when writing to file\n");
                close(fdIn);
                close(fdOut);
                return -1;
            }
            if (currDatablock.blockNum == 1) {
                catalog.fat[currDatablock.fatIndex].dataBlock1Offset = currentOffset;
            } else if (currDatablock.blockNum == 2) {
                catalog.fat[currDatablock.fatIndex].dataBlock2Offset = currentOffset;
            } else {
                catalog.fat[currDatablock.fatIndex].dataBlock3Offset = currentOffset;
            }
        }
        currentOffset = currDatablock.offset + currDatablock.size + 1;
    }
    if (writeCatalogToVault(fdOut, catalog) == -1) {
        printf("Error when writing to file\n");
        close(fdIn);
        close(fdOut);
        return -1;
    }
    close(fdIn);
    close(fdOut);
    return 0;
}

float getFragRatio(Catalog catalog) {
    off_t firstOffset;
    off_t lastOffset;
    off_t currOffset;
    off_t delta;
    off_t totalGapOffsets = 0;
    Datablock currDatablock;
    int i;
    Datablock *datablocks = getDatablocksByOffset(catalog);
    firstOffset = datablocks[0].isFree ? -1 : datablocks[0].offset;
    if (firstOffset == -1) {
        return 0;
    }
    currOffset = firstOffset + datablocks[0].size + 1;
    for (i = 1; i < 300; i++) {
        currDatablock = datablocks[i];
        if (currDatablock.isFree) {
            break;
        }
        delta = currDatablock.offset - currOffset;
        if (delta > 0) {
            totalGapOffsets += delta;
        }
        currOffset = currDatablock.offset + currDatablock.size + 1;
    }
    lastOffset = datablocks[i].isFree ? datablocks[i - 1].offset + datablocks[i - 1].size - 1 : datablocks[i].offset +
                                                                                                datablocks[i].size - 1;
    return ((float) totalGapOffsets / (lastOffset - firstOffset));
}

int status(char *vaultName) {
    Catalog catalog;
    int vaultFd = open(vaultName, O_RDONLY);
    if (vaultFd < 0) {
        printf("Error when opening file\n");
        return -1;
    }
    if (getCatalogFromFile(vaultFd, &catalog) == -1) {
        printf("Can't get catalog from file\n");
        close(vaultFd);
        return -1;
    }

    float fragRatio = getFragRatio(catalog);
    int numOfFiles = catalog.metadata.numOfFiles;
    size_t totalSize = 0;
    for (int i = 0; i < 100; i++) {
        if (!catalog.fat[i].isEmpty) {
            totalSize += catalog.fat[i].size;
        }
    }
    char sizeInString[1024];
    sizeToString(totalSize, sizeInString);

    printf("Number of files:\t%d\nTotal size:\t%s\nFragmentation ratio:\t%.2f\n", numOfFiles, sizeInString, fragRatio);
};

int main(int argc, char **argv) {

    char vaultName[256];
    char instruction[256];
    struct timeval start, end;
    long seconds, useconds;
    double mtime;


    if (argc < 3) {
        printf("Please provide arguments\n");
        return 0;
    }
    strcpy(vaultName, argv[1]);
    strcpy(instruction, argv[2]);
    //CREDIT convert string to lowercase: http://stackoverflow.com/questions/2661766/c-convert-a-mixed-case-string-to-all-lower-case
    for (int i = 0; instruction[i]; i++) {
        instruction[i] = tolower(instruction[i]);
    }


    if (strcmp(instruction, "init") == 0) {
        if (argc < 4) {
            printf("Not enough arguments for instruction\n");
        }
        char size[256];
        strcpy(size, argv[3]);

        gettimeofday(&start, NULL);

        int res = init(vaultName, size);

        gettimeofday(&end, NULL);
        mtime = ((seconds) * 1000 + useconds / 1000.0);
        printf("Elapsed time: %.3f milliseconds\n", mtime);
        return 0;
    }

    if (strcmp(instruction, "list") == 0) {

        gettimeofday(&start, NULL);

        int res = listFiles(vaultName);

        gettimeofday(&end, NULL);
        mtime = ((seconds) * 1000 + useconds / 1000.0);
        printf("Elapsed time: %.3f milliseconds\n", mtime);
        return 0;
    }

    if (strcmp(instruction, "add") == 0) {
        if (argc < 4) {
            printf("Not enough arguments for instruction\n");
        }
        char path[256];
        strcpy(path, argv[3]);

        gettimeofday(&start, NULL);

        int res = insertFile(vaultName, path);

        gettimeofday(&end, NULL);
        mtime = ((seconds) * 1000 + useconds / 1000.0);
        printf("Elapsed time: %.3f milliseconds\n", mtime);
        return 0;
    }

    if (strcmp(instruction, "rm") == 0) {
        if (argc < 4) {
            printf("Not enough arguments for instruction\n");
        }
        char fileName[256];
        strcpy(fileName, argv[3]);

        gettimeofday(&start, NULL);

        int res = deleteFile(vaultName, fileName);

        gettimeofday(&end, NULL);
        mtime = ((seconds) * 1000 + useconds / 1000.0);
        printf("Elapsed time: %.3f milliseconds\n", mtime);
        return 0;
    }

    if (strcmp(instruction, "fetch") == 0) {
        if (argc < 4) {
            printf("Not enough arguments for instruction\n");
        }
        char fileName[256];
        strcpy(fileName, argv[3]);

        gettimeofday(&start, NULL);

        int res = fetchFile(vaultName, fileName);

        gettimeofday(&end, NULL);
        mtime = ((seconds) * 1000 + useconds / 1000.0);
        printf("Elapsed time: %.3f milliseconds\n", mtime);
        return 0;
    }

    if (strcmp(instruction, "defrag") == 0) {

        gettimeofday(&start, NULL);

        int res = defrag(vaultName);

        gettimeofday(&end, NULL);
        mtime = ((seconds) * 1000 + useconds / 1000.0);
        printf("Elapsed time: %.3f milliseconds\n", mtime);
        return 0;
    }

    if (strcmp(instruction, "status") == 0) {

        gettimeofday(&start, NULL);

        int res = status(vaultName);

        gettimeofday(&end, NULL);
        mtime = ((seconds) * 1000 + useconds / 1000.0);
        printf("Elapsed time: %.3f milliseconds\n", mtime);
        return 0;
    }

    printf("Invalid instruction\n");
    return -1;
//
//
//    init("./hello.txt", "1M");
//    insertFile("./hello.txt", "./hello2.txt");
//    insertFile("./hello.txt", "./hello3.txt");
//    insertFile("./hello.txt", "./hello4.txt");
//    listFiles("./hello.txt");
//    status("./hello.txt");
//    deleteFile("./hello.txt", "hello3.txt");
//    status("./hello.txt");
//    defrag("./hello.txt");
//    status("./hello.txt");
//    fetchFile("./hello.txt", "hello2.txt");
//    Catalog catalog1;
//    getCatalogFromFile(open("./hello.txt", O_RDONLY), &catalog1);
}