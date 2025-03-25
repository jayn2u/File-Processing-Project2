# 프로젝트 2: Flash Device Driver 활용

## 개요
이 프로젝트는 Flash device driver에 대한 이해를 높이고 이를 활용하는 프로그램을 구현하는 것을 목표로 합니다. 구현 시 다음 제약 사항을 준수해야 합니다.

- 파일 I/O 연산은 system call 또는 C 라이브러리만을 사용합니다.
- 아래의 (1), (2), (3), (4), (5)의 기능을 `ftlmgr.c`에 구현합니다.
- (2), (3), (4), (5)의 기능은 `fdevicedriver.c`의 인터페이스를 이용하여 구현합니다.
- `fdevicedriver.c`는 제공된 그대로 사용하며 수정해서는 안 됩니다.

## 기능 요구사항
1. **Flash Memory Emulator**
   - Flash memory 저장 장치를 모방하는 파일을 생성합니다.
   - 블록과 페이지로 구성된 구조를 갖추며, 모든 바이트를 `0xFF`로 초기화합니다.
   - 사용 예시:  
     ```sh
     a.out c flashmemory 10
     ```
   - 10개의 블록을 갖는 `flashmemory` 파일을 생성합니다.

2. **페이지 쓰기**
   - Flash memory에 페이지 단위로 데이터를 저장합니다.
   - 실행 예시:
     ```sh
     a.out w flashmemory 5 "abcd12345@%$" "12"
     ```
   - 페이지의 섹터영역과 스페어영역을 각각 저장합니다.

3. **페이지 읽기**
   - Flash memory의 특정 페이지 데이터를 출력합니다.
   - 실행 예시:
     ```sh
     a.out r flashmemory 5
     ```
   - 저장된 데이터를 화면에 출력합니다.

4. **블록 소거 (Erase)**
   - 특정 블록을 초기화합니다.
   - 실행 예시:
     ```sh
     a.out e flashmemory 3
     ```
   - 해당 블록을 `0xFF`로 초기화합니다.

5. **In-place Update**
   - 페이지 데이터의 갱신을 수행합니다.
   - 실행 예시:
     ```sh
     a.out u flashmemory 11 "abcd6789@%$" "30"
     ```
   - 갱신 과정에서 최소한의 읽기, 쓰기, 소거 횟수를 출력합니다.

## 개발 환경
- OS: Ubuntu 22.04 LTS
- 컴파일러: `gcc 13.2`

## 제출물
- `ftlmgr.c` 소스 코드
- 제출 형식:
  - 모든 파일을 ZIP으로 압축 (`학번_2.zip`)
  - 예시: `20201084_2.zip`
- 제출 장소: 스마트캠퍼스 (lms.ssu.ac.kr)
