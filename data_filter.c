#include <stdio.h>
#include <fcntl.h>
#include <memory.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>


int main(int argc, char **argv) {

    if (argc < 4) {
        printf("Not enough arguments\n");
        return -1;
    }
    const int MAX_SIZE_FOR_SMALL_BUFFER = 10000 * 1024;
    const int SMALL_BUFFER = 1024;
    const int LARGE_BUFFER = 4 * 1024;
    char *dataAmount = argv[1];
    char *inputFilename = argv[2];
    char *outputFilename = argv[3];

    // buffer logic
    char dataAmountSizeSuffix = dataAmount[(int) strlen(dataAmount) - 1];
    int dataAmountSize = atoi(dataAmount);
    size_t dataAmountSizeBytes;
    size_t selectedBuffer;

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

    selectedBuffer = dataAmountSizeBytes < MAX_SIZE_FOR_SMALL_BUFFER ? SMALL_BUFFER : LARGE_BUFFER;
    char inBuffer[selectedBuffer + 1];
    char outBuffer[selectedBuffer + 1];
    // end of buffer logic


    // CREDIT - recitation 2
    int fdIn = open(inputFilename, O_RDONLY);
    if (fdIn < 0) {
        printf("Error opening input file: %s\n", strerror(errno));
        return errno;
    }
    // CREDIT create file if not exists, overwrite if exists: http://stackoverflow.com/questions/23092040/how-to-open-a-file-which-overwrite-existing-content
    int fdOut = open(outputFilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    if (fdOut < 0) {
        close(fdIn);
        printf("Error opening output file: %s\n", strerror(errno));
        return errno;
    }
    size_t outBufferIndex = 0;
    size_t totalBytesRead = 0;
    size_t numOfPrintable = 0;
    ssize_t bytesRead = read(fdIn, inBuffer, selectedBuffer);
    if (bytesRead == -1) {
        close(fdIn);
        close(fdOut);
        printf("Error reading file\n");
        return -1;
    }
    ssize_t bytesWritten;
    bool maxBytesReached = false;
    while (bytesRead > 0 && !maxBytesReached) {
        if (totalBytesRead + bytesRead > dataAmountSizeBytes) {
            bytesRead = dataAmountSizeBytes - totalBytesRead;
            maxBytesReached = true;
        }
        totalBytesRead += bytesRead;
        int i;
        for (i = 0; i < bytesRead; i++) {
            if (inBuffer[i] < 127 && inBuffer[i] > 31) {
                outBuffer[outBufferIndex] = inBuffer[i];
                numOfPrintable++;
                outBufferIndex++;
                if (outBufferIndex == selectedBuffer) {
                    outBuffer[selectedBuffer] = '\0';
                    bytesWritten = write(fdOut, outBuffer, selectedBuffer);
                    if (bytesWritten == -1 || bytesWritten != selectedBuffer) {
                        close(fdIn);
                        close(fdOut);
                        printf("Error when writing to file\n");
                        return -1;
                    }
                    outBufferIndex = 0;
                }
            }
        }
        bytesRead = read(fdIn, inBuffer, selectedBuffer - 1);
        if (bytesRead == -1) {
            close(fdIn);
            close(fdOut);
            printf("Error reading file\n");
            return -1;
        }
        if (bytesRead == 1 && totalBytesRead < dataAmountSizeBytes) {
            lseek(fdIn, 0, SEEK_SET);
            bytesRead = read(fdIn, inBuffer, selectedBuffer - 1);
            if (bytesRead == -1) {
                close(fdIn);
                close(fdOut);
                printf("Error reading file\n");
                return -1;
            }
        }

    }
    if (outBufferIndex > 0) {
        outBuffer[outBufferIndex] = '\0';
        bytesWritten = write(fdOut, outBuffer, outBufferIndex);
        if (bytesWritten == -1 || bytesWritten != outBufferIndex) {
            close(fdIn);
            close(fdOut);
            printf("Error when writing to file\n");
            return -1;
        }
    }
    //CREDIT printing size_t: https://0x657573.wordpress.com/2010/11/22/printing-size_t-or-ssize_t-variable/
    printf("%zd characters requested, %zd characters read, %zd are printable\n", dataAmountSizeBytes, totalBytesRead,
           numOfPrintable);
    close(fdIn);
    close(fdOut);
    return 0;
}
