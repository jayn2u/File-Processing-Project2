cmake_minimum_required(VERSION 3.30)
project(FileProcessingProject2 C)

set(CMAKE_C_STANDARD 11)

add_executable(FileProcessingProject2 ftlmgr.c fdevicedriver.c
        fdevicedriver.h)
