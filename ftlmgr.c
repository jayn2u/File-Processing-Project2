#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "flash.h"
#include "fdevicedriver.h"
// 필요한 경우 헤더파일을 추가한다

FILE *flashmemoryfp; // fdevicedriver.c에서 사용

int create_flashmemory_emulator(char *filename, char *blockbuf, int block_num);

int write_pages(char *pagebuf, char *flashfile, int ppn, const char *sectordata, const char *sparedata);

int read_pages(char *argv[], char *pagebuf, char *sectorbuf, char *sparebuf);

int erase_block(char *flashfile, int pbn);

int inplace_update(char *argv[], char *pagebuf, char *sectorbuf, char *sparebuf);

//
// 이 함수는 FTL의 역할 중 일부분을 수행하는데 물리적인 저장장치 flash memory에 Flash device driver를 이용하여 데이터를
// 읽고 쓰거나 블록을 소거하는 일을 한다 (강의자료 참조).
// flash memory에 데이터를 읽고 쓰거나 소거하기 위해서 fdevicedriver.c에서 제공하는 인터페이스를
// 호출하면 된다. 이때 해당되는 인터페이스를 호출할 때 연산의 단위를 정확히 사용해야 한다.
// 읽기와 쓰기는 페이지 단위이며 소거는 블록 단위이다 (주의: 읽기, 쓰기, 소거를 하기 위해서는 반드시 fdevicedriver.c의 함수를 사용해야 함)
// 갱신은 강의자료의 in-place update를 따른다고 가정한다.
// 스페어영역에 저장되는 정수는 하나이며, 반드시 binary integer 모드로 저장해야 한다.
//
int main(int argc, char *argv[]) {
    char sectorbuf[SECTOR_SIZE];
    memset(sectorbuf, '\0', SECTOR_SIZE);
    char sparebuf[SPARE_SIZE];
    memset(sparebuf, '\0', SPARE_SIZE);
    char pagebuf[PAGE_SIZE];
    char *blockbuf;

    // flash memory 파일 생성: 위에서 선언한 flashmemoryfp를 사용하여 플래시 메모리 파일을 생성한다. 그 이유는 fdevicedriver.c에서
    //                 flashmemoryfp 파일포인터를 extern으로 선언하여 사용하기 때문이다.
    // 페이지 쓰기: pagebuf의 섹터와 스페어에 각각 입력된 데이터를 정확히 저장하고 난 후 해당 인터페이스를 호출한다
    // 페이지 읽기: pagebuf를 인자로 사용하여 해당 인터페이스를 호출하여 페이지를 읽어 온 후 여기서 섹터 데이터와
    //                  스페어 데이터를 분리해 낸다
    // memset(), memcpy() 등의 함수를 이용하면 편리하다. 물론, 다른 방법으로 해결해도 무방하다.

    int ret = 0;

    switch (argv[1][0]) {
        case 'c':
            ret = create_flashmemory_emulator(argv[2], blockbuf, atoi(argv[3]));
            if (ret != EXIT_SUCCESS) {
                fprintf(stderr, "플래시 메모리 생성 간 문제 발생\n");
                return EXIT_FAILURE;
            }
            break;

        case 'w':
            ret = write_pages(pagebuf, argv[2], atoi(argv[3]), argv[4], argv[5]);
            if (ret != EXIT_SUCCESS) {
                fprintf(stderr, "페이지 쓰기 간 문제 발생\n");
                return EXIT_FAILURE;
            }
            break;

        case 'r':
            ret = read_pages(argv, pagebuf, sectorbuf, sparebuf);
            if (ret != EXIT_SUCCESS) {
                fprintf(stderr, "페이지 읽기 간 문제 발생\n");
                return EXIT_FAILURE;
            }
            if (strlen(sectorbuf) > 0 || strlen(sparebuf) > 0) {
                printf("%s %s\n", sectorbuf, sparebuf);
            }
            break;

        case 'e':
            ret = erase_block(argv[2], atoi(argv[3]));
            if (ret != EXIT_SUCCESS) {
                fprintf(stderr, "블록 소거 간 문제 발생\n");
                return EXIT_FAILURE;
            }
            break;

        case 'u':
            ret = inplace_update(argv, pagebuf, sectorbuf, sparebuf);
            if (ret != EXIT_SUCCESS) {
                fprintf(stderr, "In-place 업데이트 간 문제 발생\n");
                return EXIT_FAILURE;
            }
            break;

        default:
            fprintf(stderr, "옵션을 지정하지 않았습니다.\n");
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int create_flashmemory_emulator(char *filename, char *blockbuf, int block_num) {
    char *flashfile = filename;
    const int num_blocks = block_num;

    if (num_blocks <= 0) {
        fprintf(stderr, "블록의 수는 0보다 작으면 안됩니다.\n");
        return EXIT_FAILURE;
    }

    // Flash memory 파일 생성
    flashmemoryfp = fopen(flashfile, "wb");
    if (flashmemoryfp == NULL) {
        fprintf(stderr, "플래시메모리 파일 생성에 실패했습니다.\n");
        return EXIT_FAILURE;
    }

    // 각 블록을 0xFF로 초기화
    blockbuf = (char *) malloc(num_blocks * BLOCK_SIZE);
    if (blockbuf == NULL) {
        fprintf(stderr, "블록버퍼 생성에 실패했습니다.\n");
        fclose(flashmemoryfp);
        return EXIT_FAILURE;
    }

    memset(blockbuf, (char)0xFF, num_blocks * BLOCK_SIZE);

    // 블록 수만큼 파일에 쓰기
    for (int i = 0; i < num_blocks; i++) {
        if (fwrite(blockbuf, BLOCK_SIZE, 1, flashmemoryfp) != 1) {
            fprintf(stderr, "%d 블록에 초기화 실패했습니다.\n", i);
            free(blockbuf);
            fclose(flashmemoryfp);
            return EXIT_FAILURE;
        }
    }

    free(blockbuf);
    fclose(flashmemoryfp);
    return EXIT_SUCCESS;
}

int write_pages(char *pagebuf, char *flashfile, int ppn, const char *sectordata, const char *sparedata) {
    flashmemoryfp = fopen(flashfile, "wb");
    if (flashmemoryfp == NULL) {
        fprintf(stderr, "flashmemoryfp 파일 열기에 실패했습니다.\n");
        return EXIT_FAILURE;
    }

    memset(pagebuf, 0, PAGE_SIZE);
    memcpy(pagebuf, sectordata, strlen(sectordata));
    memcpy(pagebuf + SECTOR_SIZE, sparedata, strlen(sparedata));
    // printf("[DEBUG] sector_data in pagebuf: %s\n", pagebuf);
    // printf("[DEBUG] spare_data in pagebuf + SECTOR_SIZE: %s\n", pagebuf + SECTOR_SIZE);

    fdd_write(ppn, pagebuf);

    fclose(flashmemoryfp);
    return EXIT_SUCCESS;
}

int read_pages(char *argv[], char *pagebuf, char *sectorbuf, char *sparebuf) {
    flashmemoryfp = fopen(argv[2], "rb");
    if (flashmemoryfp == NULL) {
        fprintf(stderr, "flashmemoryfp 파일 열기에 실패했습니다.\n");
        return EXIT_FAILURE;
    }

    int ppn = atol(argv[3]);

    fdd_read(ppn, pagebuf);

    int is_erased = 1;
    for (int i = 0; i < PAGE_SIZE; i++) {
        if ((unsigned char) pagebuf[i] != 0xFF) {
            is_erased = 0;
            break;
        }
    }
    if (is_erased) {
        return EXIT_SUCCESS;
    }

    memcpy(sectorbuf, pagebuf, strlen(pagebuf));
    memcpy(sparebuf, pagebuf + SECTOR_SIZE, strlen(pagebuf + SECTOR_SIZE));

    fclose(flashmemoryfp);
    return EXIT_SUCCESS;
}

int erase_block(const char *flashfile, int pbn) {
    flashmemoryfp = fopen(flashfile, "rb+");
    if (flashmemoryfp == NULL) {
        fprintf(stderr, "flashmemoryfp 파일 열기에 실패했습니다.\n");
        return EXIT_FAILURE;
    }

    fdd_erase(pbn);

    fclose(flashmemoryfp);
    return EXIT_SUCCESS;
}

int inplace_update(char *argv[], char *pagebuf, char *sectorbuf, char *sparebuf) {
    flashmemoryfp = fopen(argv[2], "rb+");
    if (flashmemoryfp == NULL) {
        fprintf(stderr, "flashmemoryfp 파일 열기에 실패했습니다.\n");
        return EXIT_FAILURE;
    }

    int ret = 0;

    ret = read_pages(argv, pagebuf, sectorbuf, sparebuf);
    if (ret != EXIT_SUCCESS) {
        fprintf(stderr, "In-place 업데이트 중 페이지 읽기 간 문제가 발생했습니다.\n");
        fclose(flashmemoryfp);
        return EXIT_FAILURE;
    }

    // 새로 입력받은 데이터로 업데이트
    ret = write_pages(pagebuf, argv[2], atoi(argv[3]), argv[4], argv[5]);
    if (ret != EXIT_SUCCESS) {
        fprintf(stderr, "In-place 업데이트 중 페이지 쓰기 간 문제가 발생했습니다.\n");
        fclose(flashmemoryfp);
        return EXIT_FAILURE;
    }

    fclose(flashmemoryfp);
    return EXIT_SUCCESS;
}
