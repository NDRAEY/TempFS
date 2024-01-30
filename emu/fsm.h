#pragma once

#include <stdint.h>

typedef struct {
	uint16_t year;	///< ���
	uint8_t month;	///< �����
	uint8_t day;	///< ����
	uint8_t hour;	///< ���
	uint8_t minute;	///< ������
	uint8_t second;	///< �������
} __attribute__((packed)) FSM_TIME;

typedef struct {
	int Ready;				///< ���������� �� ����?
    char Name[1024];		///< ��� �����
    char Path[1024];		///< ���� �����
    int Mode;				///< ����� �����
    size_t Size;			///< ������ ����� � ������ (o�t2bin)
    FSM_TIME LastTime;		///< ����� ���������� ��������� �����
    int Type;				///< ��� ��������
    uint8_t CHMOD;			///< CHMOD �����
} __attribute__((packed)) FSM_FILE;

typedef struct {
	int Ready;				///< ���������� �� �����?
	size_t Count;			///< ���������� �����
	size_t CountFiles;		///< ���������� ������
	size_t CountDir;		///< ���������� �����
	size_t CountOther;		///< ���������� ������������ ���� ������
    FSM_FILE* Files;		///< ����� � �����
} __attribute__((packed)) FSM_DIR;
