#pragma once



typedef struct {
	bool Ready;				///< ���������� ����������? (1 - �� | 0 - ���)
	void* Reserved;			///< ����� � ��� �������� ���.������ ���� ���������.
} __attribute__((packed)) DPM_Disk;

void dpm_init(const char* path);
size_t dpm_write(char Letter, size_t Offset, size_t Size, void *Buffer);
size_t dpm_read(char Letter, size_t Offset, size_t Size, void *Buffer);
size_t dpm_get_size(char Letter);
int fsm_isPathToFile(const char* Path,const char* Name);
