#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "flash.h"
#include "fdevicedriver.h"
// 필요한 경우 헤더파일을 추가한다

FILE *flashmemoryfp; // fdevicedriver.c에서 사용

int create_flashmemory_emulator(char *argv[], char *blockbuf);

int write_pages(char *argv[], char *pagebuf);

int read_pages(char *argv[], char *pagebuf);

int erase_block(char *argv[]);

int inplace_update(char *argv[], char *pagebuf);

int find_target_block(int ppn, int total_blocks_cnt, char *pagebuf);

int is_block_empty(int block_index, char *pagebuf);

int get_total_blocks(const char *filename);

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
    char sectorbuf;
    char sparebuf;
    char pagebuf[PAGE_SIZE];
    char *blockbuf;

    memset(pagebuf, (char)0xFF, PAGE_SIZE);

    // flash memory 파일 생성: 위에서 선언한 flashmemoryfp를 사용하여 플래시 메모리 파일을 생성한다. 그 이유는 fdevicedriver.c에서
    //                 flashmemoryfp 파일포인터를 extern으로 선언하여 사용하기 때문이다.
    // 페이지 쓰기: pagebuf의 섹터와 스페어에 각각 입력된 데이터를 정확히 저장하고 난 후 해당 인터페이스를 호출한다
    // 페이지 읽기: pagebuf를 인자로 사용하여 해당 인터페이스를 호출하여 페이지를 읽어 온 후 여기서 섹터 데이터와
    //                  스페어 데이터를 분리해 낸다
    // memset(), memcpy() 등의 함수를 이용하면 편리하다. 물론, 다른 방법으로 해결해도 무방하다.

    int ret;

    switch (argv[1][0]) {
        case 'c':
            ret = create_flashmemory_emulator(argv, blockbuf);
            if (ret != EXIT_SUCCESS) {
                fprintf(stderr, "플래시 메모리 생성 간 문제 발생\n");
                return EXIT_FAILURE;
            }
            break;

        case 'w':
            ret = write_pages(argv, pagebuf);
            if (ret != EXIT_SUCCESS) {
                fprintf(stderr, "페이지 쓰기 간 문제 발생\n");
                return EXIT_FAILURE;
            }
            break;

        case 'r':
            ret = read_pages(argv, pagebuf);
            if (ret != EXIT_SUCCESS) {
                fprintf(stderr, "페이지 읽기 간 문제 발생\n");
                return EXIT_FAILURE;
            }
            break;

        case 'e':
            ret = erase_block(argv);
            if (ret != EXIT_SUCCESS) {
                fprintf(stderr, "블록 소거 간 문제 발생\n");
                return EXIT_FAILURE;
            }
            break;

        case 'u':
            ret = inplace_update(argv, pagebuf);
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

int create_flashmemory_emulator(char *argv[], char *blockbuf) {
    char *flashfile = argv[2];
    const int num_blocks = atoi(argv[3]);

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
        fprintf(stderr, "블록퍼퍼 생성에 실패했습니다.\n");
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

int write_pages(char *argv[], char *pagebuf) {
    flashmemoryfp = fopen(argv[2], "rb+");
    if (flashmemoryfp == NULL) {
        fprintf(stderr, "flashmemoryfp 파일 열기에 실패했습니다.\n");
        return EXIT_FAILURE;
    }

    int ppn = atoi(argv[3]);

    char *sector_start_address = pagebuf;
    char *spare_start_address = sector_start_address + SECTOR_SIZE;

    memcpy(sector_start_address, argv[4], strlen(argv[4]));
    memcpy(spare_start_address, argv[5], strlen(argv[5]));

    fdd_write(ppn, pagebuf);

    fclose(flashmemoryfp);
    return EXIT_SUCCESS;
}

int read_pages(char *argv[], char *pagebuf) {
    flashmemoryfp = fopen(argv[2], "rb+");
    if (flashmemoryfp == NULL) {
        fprintf(stderr, "flashmemoryfp 파일 열기에 실패했습니다.\n");
        return EXIT_FAILURE;
    }

    int ppn = atoi(argv[3]);
    fdd_read(ppn, pagebuf);

    // 섹터 영역에서 0xFF가 아닌 바이트만 필터링
    char nonErasedSector[SECTOR_SIZE + 1];
    int index = 0;
    for (int i = 0; i < SECTOR_SIZE; i++) {
        if ((unsigned char) pagebuf[i] != 0xFF) {
            nonErasedSector[index++] = pagebuf[i];
        }
    }
    nonErasedSector[index] = '\0';

    // 스페어 영역에서 0xFF가 아닌 바이트만 필터링
    char nonErasedSpare[SPARE_SIZE + 1];
    index = 0;
    for (int i = SECTOR_SIZE; i < PAGE_SIZE; i++) {
        if ((unsigned char) pagebuf[i] != 0xFF) {
            nonErasedSpare[index++] = pagebuf[i];
        }
    }
    nonErasedSpare[index] = '\0';

    // 필터링한 결과가 모두 빈 문자열인 경우, 페이지가 초기화된 것으로 판단
    if (strlen(nonErasedSector) == 0 && strlen(nonErasedSpare) == 0) {
        fclose(flashmemoryfp);
        return EXIT_SUCCESS;
    }

    printf("%s %s\n", nonErasedSector, nonErasedSpare);

    fclose(flashmemoryfp);
    return EXIT_SUCCESS;
}

int erase_block(char *argv[]) {
    flashmemoryfp = fopen(argv[2], "rb+");
    if (flashmemoryfp == NULL) {
        fprintf(stderr, "flashmemoryfp 파일 열기에 실패했습니다.\n");
        return EXIT_FAILURE;
    }

    const int pbn = atoi(argv[3]);

    fdd_erase(pbn);

    fclose(flashmemoryfp);
    return EXIT_SUCCESS;
}

int inplace_update(char *argv[], char *pagebuf) {
    flashmemoryfp = fopen(argv[2], "rb+");
    if (flashmemoryfp == NULL) {
        fprintf(stderr, "flashmemoryfp 파일 열기에 실패했습니다.\n");
        return EXIT_FAILURE;
    }

    int ppn = atoi(argv[3]);
    int pbn = ppn / PAGE_NUM;
    int offset = ppn % PAGE_NUM;
    char *new_sector = argv[4];
    char *new_spare = argv[5];

    // 블록의 모든 페이지를 메모리에 읽기
    char block_pages[PAGE_NUM][PAGE_SIZE];
    int reads = 0;
    for (int i = 0; i < PAGE_NUM; i++) {
        int current_ppn = pbn * PAGE_NUM + i;
        if (fdd_read(current_ppn, block_pages[i]) != 1) {
            fprintf(stderr, "페이지 읽기 실패: %d\n", current_ppn);
            fclose(flashmemoryfp);
            return EXIT_FAILURE;
        }
        reads++;
    }

    // 블록 지우기
    int erases = 0;
    if (fdd_erase(pbn) != 1) {
        fprintf(stderr, "블록 소거 실패: %d\n", pbn);
        fclose(flashmemoryfp);
        return EXIT_FAILURE;
    }
    erases++;

    // 업데이트된 페이지 준비
    char updated_page[PAGE_SIZE];
    memset(updated_page, 0xFF, PAGE_SIZE);
    int len = strlen(new_sector);
    if (len > SECTOR_SIZE) len = SECTOR_SIZE;
    memcpy(updated_page, new_sector, len);
    int spare_len = strlen(new_spare);
    if (spare_len > SPARE_SIZE) spare_len = SPARE_SIZE;
    memcpy(updated_page + SECTOR_SIZE, new_spare, spare_len);

    // 모든 페이지를 원래 위치에 쓰기
    int writes = 0;
    for (int i = 0; i < PAGE_NUM; i++) {
        int current_ppn = pbn * PAGE_NUM + i;
        if (i == offset) {
            if (fdd_write(current_ppn, updated_page) != 1) {
                fprintf(stderr, "업데이트 페이지 쓰기 실패: %d\n", current_ppn);
                fclose(flashmemoryfp);
                return EXIT_FAILURE;
            }
        } else {
            if (fdd_write(current_ppn, block_pages[i]) != 1) {
                fprintf(stderr, "페이지 쓰기 실패: %d\n", current_ppn);
                fclose(flashmemoryfp);
                return EXIT_FAILURE;
            }
        }
        writes++;
    }

    printf("#reads=%d #writes=%d #erases=%d", reads, writes, erases);
    fclose(flashmemoryfp);
    return EXIT_SUCCESS;
}

// argv에서 전달된 ppn 값에 해당하는 블록이 비어있다면 바로 반환하고,
// 그렇지 않으면 다른 빈 블록을 찾는 함수
int find_target_block(int ppn, int total_blocks_cnt, char *pagebuf) {
    int given_block = ppn / PAGE_NUM;

    // 먼저, ppn이 속한 블록이 비어있는지 확인
    if (is_block_empty(given_block, pagebuf)) {
        return given_block;
    }

    // 만약 해당 블록이 비어있지 않다면 다른 빈 블록 탐색 (given_block은 제외)
    for (int block_index = 0; block_index < total_blocks_cnt; block_index++) {
        if (block_index == given_block) {
            continue;
        }
        if (is_block_empty(block_index, pagebuf)) {
            return block_index;
        }
    }

    // 빈 블록을 찾지 못한 경우
    return -1;
}

// 블록이 비어있는지 검사하는 함수
// block_index: 검사할 블록 번호, pagebuf: 페이지 데이터 임시 버퍼
int is_block_empty(int block_index, char *pagebuf) {
    int block_empty = 1; // 블록이 비어있다고 가정
    for (int page_offset = 0; page_offset < PAGE_NUM; page_offset++) {
        int ppn = block_index * PAGE_NUM + page_offset;
        if (fdd_read(ppn, pagebuf) == -1) {
            fprintf(stderr, "페이지 %d 읽기 실패\n", ppn);
            return 0; // 오류 발생 시 비어있지 않다고 간주
        }
        for (int i = 0; i < PAGE_SIZE; i++) {
            if ((unsigned char) pagebuf[i] != 0xFF) {
                return 0; // 하나라도 초기화되지 않은 데이터가 있으면 비어있지 않음
            }
        }
    }
    return 1;
}

int get_total_blocks(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "파일 열기에 실패했습니다: %s\n", filename);
        return -1; // 오류 처리
    }

    // 파일의 끝으로 이동 후, 파일 크기를 측정
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    fclose(fp);

    // 파일 크기가 BLOCK_SIZE의 배수가 아닌 경우, 경고 혹은 오류 처리 가능
    if (filesize % BLOCK_SIZE != 0) {
        fprintf(stderr, "파일 크기가 BLOCK_SIZE의 배수가 아닙니다.\n");
        return -1;
    }

    int total_blocks_cnt = filesize / BLOCK_SIZE;
    return total_blocks_cnt;
}
