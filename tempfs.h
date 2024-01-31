#pragma once

#include <stdint.h>

#define TEMPFS_CHMOD_READ               0x01  /// ����� ������
#define TEMPFS_CHMOD_WRITE              0x02  /// ����� ������
#define TEMPFS_CHMOD_EXEC               0x04  /// ����� ����������
#define TEMPFS_CHMOD_SYS                0x08  /// ����� �������


#define TEMPFS_DIR_INFO_ROOT            0x01  /// ������������ ����� �������
#define TEMPFS_DIR_INFO_EXITS           0x02  /// ����� �������

#define TEMPFS_ENTITY_STATUS_ERROR      0x00  /// �������� �� ������ � ������
#define TEMPFS_ENTITY_STATUS_READY      0x01  /// �������� ������ � ������ (entity)
#define TEMPFS_ENTITY_STATUS_PKG_READY  0x02  /// �������� ������ � ������ (package)

#define TEMPFS_ENTITY_TYPE_UNKNOWN      0x00  /// ����������
#define TEMPFS_ENTITY_TYPE_FILE         0x01  /// ����
#define TEMPFS_ENTITY_TYPE_FOLDER       0x02  /// �����


typedef struct {
	uint8_t Status;			///< ������
    char Name[64];		    ///< ��� �����
    char Path[412];		    ///< ���� �����
    uint32_t Size;			///< ������ ����� � ������
    uint32_t Point;			///< ����� ����� � ����
    uint32_t Date;			///< ���� ���������
    char Owner[16];		    ///< �������� �����
	uint8_t Type;			///< ��� �����
	uint8_t CHMOD;			///< ����� �������
} TEMPFS_ENTITY;  // ��������� 512 ����

typedef struct {
	uint16_t Sign1;			///< ��������� 1
	uint16_t Sign2;			///< ��������� 2
    char Label[32];		    ///< ����� �����
    uint32_t CountFiles;	///< ���������� ������
    uint32_t EndDisk;		///< ����� ����� �����
    uint32_t CountBlocks;	///< ���������� ������������ ������
	char Rev[16];			///< �� ������������
}  TEMPFS_BOOT;

typedef struct {
	uint8_t Status;					///< ������ ������
	uint16_t Length;				///< ����� ������
	uint32_t Next;					///< ��������� ����� ������
	char Data[500];					///< ����� ������
	uint8_t Rev;					///< ���������������, ������ ����
} TEMPFS_PACKAGE;  // ��������� 512 ����

typedef struct {
	uint8_t Status;			    ///< ������
	TEMPFS_BOOT* Boot;			///< ������ �� Boot
    uint32_t CountFiles;	    ///< ���������� ������
	TEMPFS_ENTITY* Files;		///< ������ �� �������� ������
    uint32_t BlocksAll;	        ///< ������������ ���������� ������ ����������
    uint32_t FreeAll;           ///< ��������� ���������� ������
    uint32_t EndPoint;          ///< ����� ����� �����
}  TEMPFS_Cache;
