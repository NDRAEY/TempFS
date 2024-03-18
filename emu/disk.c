#include "../port.h"

FILE *disk;
size_t disk_size = 0;

DPM_Disk DPM_Disks[32] = {0};

/**
 * @brief �������� ������� ����� �� ����
 *
 * @param Path - ����, ������� ���������
 * @param Name - ��� ����� � ������ ����� ��� ��������
 *
 * @return int - 1 ���� ���� ����
 */
int fsm_isPathToFile(const char* Path,const char* Name){
	char* zpath = pathinfo(Name, PATHINFO_DIRNAME);					///< �������� ������������ ����� ��������
	char* bpath = pathinfo(Name, PATHINFO_BASENAME);				///< �������� ��� ����� (��� ������� ���� �����)
	bool   isCheck1 = strcmpn(zpath,Path);				            ///< ��������� ���������� �����
	bool   isCheck2 = strlen(bpath) == 0;				            ///< ���������, ��� ���� ��� ������ ������� (����� 0, ���� ������ �����)
	bool   isCheck3 = str_contains(Name, Path);	                    ///< ��������� �������, ��������� ����
	size_t c1 = str_cdsp2(Path,'\\');
	size_t c2 = str_cdsp2(Name,'\\');
	size_t c3 = str_cdsp2(Path,'/');
	size_t c4 = str_cdsp2(Name,'/');

	bool   isCheck4 = ((c2 - c1) == 1) && (c4 == c3);
	bool   isCheck5 = ((c4 - c3) == 1) && (c2 == c1);
	bool isPassed = ((isCheck1 && !isCheck2 && isCheck3) || (!isCheck1 && isCheck2 && isCheck3 && (isCheck4 || isCheck5)));
	free(zpath);
	free(bpath);
	return isPassed;
}


void get_disk_size(const char* path) {
    printf(" |--- [>] Attempt to open to get the file size from %s\n", path);
    FILE *file = fopen(path, "rb");    // ��������� ���� � ������ ������ �������� ������
    if (file == NULL){
            printf(" |     |--- [ERR] An error occurred while retrieving the file size\n");
            printf(" |\n");
            return;
    }
    fseek(file, 0, SEEK_END);               // ������������� ������� ��������� ����� � ����� �����
    disk_size = ftell(file);                // �������� ������� ������� ��������� �����, ��� �������� �������� �����
    fclose(file);                           // ��������� ����
    printf(" |     |--- [OK] File size: %d\n", (int) disk_size);
    printf(" |\n");
}


/**
 * @brief [DPM] ���������� ������ � �����
 *
 * @param Letter - ����� ��� ����������
 * @param Offset - ������ ��� ����������
 * @param Size - ���-�� ���� ������ ��� ����������
 * @param Buffer - ����� ���� ����� ���� ������
 *
 * @return ���-�� ����������� ����
 */
size_t dpm_read(char Letter, size_t Offset, size_t Size, void* Buffer){
    /// ����� � https://github.com/pimnik98/SayoriOS/blob/main/kernel/src/drv/disk/dpm.c
    /// �� � ������� ����� �������������� ����� ����� � ���������� ��� ������ � ������
    int go = fseek(disk, Offset, SEEK_SET);
    if (Offset > disk_size){
        printf("[WARN] It was not possible to move to the %d position, due to: exceeding the limits of the disk volume\n", (unsigned int) Offset);
        return 0;
    } else if (go != 0){
        printf("[WARN] Failed to move to position %d, due to: error moving using fseek\n", (unsigned int) Offset);
        return 0;
    }
    size_t read = fread(Buffer, 1,Size, disk);
    return read;
}


/**
 * @brief [DPM] ������ ������ �� ����
 *
 * @param Letter - �����
 * @param size_t Offset - ������
 * @param size_t Size - ���-�� ���� ������ ��� ������
 * @param Buffer - ����� ������ ����� ���� ������
 *
 * @return size_t - ���-�� ���������� ����
 */
size_t dpm_write(char Letter, size_t Offset, size_t Size, void* Buffer){
    /// ����� � https://github.com/pimnik98/SayoriOS/blob/main/kernel/src/drv/disk/dpm.c
    /// �� � ������� ����� �������������� ����� ����� � ���������� ��� ������ � ������
    int go = fseek(disk, Offset, SEEK_SET);
    if (Offset > disk_size){
        printf("[WARN] It was not possible to move to the %d position, due to: exceeding the limits of the disk volume\n", (unsigned int) Offset);
        return 0;
    } else if (go != 0){
        printf("[WARN] Failed to move to position %d, due to: error moving using fseek\n", (unsigned int) Offset);
        return 0;
    }
    size_t write = fwrite(Buffer, 1, Size, disk);
    return write;
}

size_t dpm_get_size(char Letter){
    return disk_size;
}

void* dpm_metadata_read(char Letter){
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	//if (DPM_Disks[Index].Ready == 0 || DPM_Disks[Index].Status == 0)
		//return 0;

	return DPM_Disks[Index].Reserved;
}

void dpm_metadata_write(char Letter, uint32_t Addr){
	int Index = Letter - 65;

	Index = (Index > 32 ? Index - 32 : Index);
	Index = (Index < 0 || Index > 25 ? 0 : Index);

	DPM_Disks[Index].Reserved = (void*)Addr;
}


void dpm_init(const char* path) {
    /// �������������� ���� ����, ��� ����� �� ����� ������������ ����
    printf("\n[>] Attempt to load disk \"%s\"\n", path);
    disk = fopen(path, "r+b");
    if (disk == NULL) {
        printf (" |--- [ERR] File disk.img no found!\n");
        exit(1);
    }
    get_disk_size(path);
    printf(" |--- [OK] Loaded disk \"%s\"\n", path);
    printf(" |--- [OK] Size disk %d b. | %d kb. | %d mb.\n", (int) disk_size, (int) disk_size/1024, (int) disk_size/1024/1024);
}
