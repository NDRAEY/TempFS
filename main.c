#include "port.h"

void fsminfo(char* Path){
    FSM_FILE file = fs_tempfs_info('T', Path);
    tfs_log(" [>] File  : %s\n",Path);
    tfs_log("  |--- Ready  : %d\n",file.Ready);
	tfs_log("  |--- Name   : %s\n",file.Name);
	tfs_log("  |--- Path   : %s\n",file.Path);
	tfs_log("  |--- Mode   : %d\n",file.Mode);
	tfs_log("  |--- Size   : %d\n",file.Size);
	tfs_log("  |--- Type   : %d\n",file.Type);
	tfs_log("  |--- Date   : %d\n",file.LastTime.year);

	if (file.Ready != 1) return;

	char* Buffer = malloc(file.Size);
	memset(Buffer, 0, file.Size);

	size_t read = fs_tempfs_read('T', Path, 500, 50, Buffer);
	tfs_log("READ: %d (%d) bytes...\n=====\n%s\n=====\n", read, file.Size, Buffer);

}


void printFiles(const char* FOLDER){

    printf("\n[>] Getting data from the %s folder \n",FOLDER);
    FSM_DIR* Dir = fs_tempfs_dir('T', FOLDER);
    if (Dir == NULL || Dir->Ready == 0){
        printf(" |--- [ERR] Failed to get folder information\n");
        free(Dir);
        return;
    }
    size_t Sizes = 0;
    for (int i = 0; i < Dir->Count; i++){
        printf("%s\t%s\t\t%s\n",
            " |--- 0000-00-00 00:00",
            (Dir->Files[i].Type == 5?"<DIR> ":"<FILE>"),
            Dir->Files[i].Name
        );
        Sizes += Dir->Files[i].Size;
    }
    printf(" |\n |--- Files: %d | Folders: %d | All: %d\n", Dir->CountFiles, Dir->CountDir, Dir->Count);
    printf(" |--- Folder size: %d mb. | %d kb. | %d b.\n", (Sizes != 0?(Sizes/1024/1024):0), (Sizes != 0?(Sizes/1024):0), Sizes);

    free(Dir);
}

int main()
{
    printf("TempFS - A simple file system with support for reading and writing files.\n");

    printf("TEMPFS_PACKAGE : %d\n", sizeof(TEMPFS_PACKAGE));

    dpm_init("disk.img");
    int detect = fs_tempfs_detect('T');
    if (detect == 0){
        printf(" |--- [ERR] Could not verify validity on TempFS\n");
        fs_tempfs_format('T'); ///< ��� ������ ����������� ���� ��� �������� TempFS
        return 1;
    }
    printf(" |--- [OK] Successful validation of TempFS\n");



    char* label = malloc(32);
    memset(label, 0 , 32);
    fs_tempfs_label('T',label);

    printf(" |--- Label: %s\n",label);

    printFiles("/");
    printFiles("/datafilefs/");


    //fs_tempfs_delete('T',"/", 1);
    //fs_tempfs_delete('T',"/datafilefs/temp.txt", 0);
    //fs_tempfs_delete('T',"/datafilefs/", 1);



    //fsminfo("/datafilefs/temp.txt");
    //fsminfo("/datafilefs/");
    //fsminfo("/test/");

/**
    int create_folder = fs_tempfs_create('T',"/datafilefs/",1);
    printf("create_folder: %d\n", create_folder);

    int create_file = fs_tempfs_create('T',"/datafilefs/temp.txt",0);
    printf("create_file: %d\n", create_file);

    int tempwr = fs_tempfs_write('T',"/datafilefs/temp.txt", 498, strlen("EllyNet"), "EllyNet");

    printf("tempwr: %d\n", tempwr);
*/
    return 0;
}
