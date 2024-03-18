cmake_minimum_required(VERSION 3.25)
project(TempFS LANGUAGES C)

set(PROJECT_FILES
	emu/disk.c
	emu/disk.h
	emu/fsm.h
	ext/my_string.c
	ext/my_string.h
	ext/pathinfo.c
	ext/pathinfo.h
	port.h
	tempfs.c
	tempfs.h
)

add_executable(tempfs ${PROJECT_FILES} main.c)
add_executable(tempfs_utils ${PROJECT_FILES} util.c)
