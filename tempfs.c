#include "port.h"

FSM_DIR* Cache = {0};

// Часть функционала будет переписана для работы именно с этим кэшем
// Если есть желание поддерживать несколько смонтированных файловых систем
// Тогда рекомендуется,сделать поддержку это внутри вашей ОС
// К примеру, в драйвере разделов, есть специальная позиция
// где можно оставить именно ссылку на этот кэш и после на него ссылаться
// просто в нужных местах замените переменную __TCache__ на необходимую


int fs_tempfs_func_getCountAllBlocks(size_t all_disk){
    size_t all_free_disk = (all_disk) - (sizeof(TEMPFS_BOOT)) - 1;                      /// Выполняем расчеты всего доступного простраства
    if (all_free_disk <= 0){                                                            /// Проверка на свободное пространство
        return 0;                                                                           /// Возвращаем 0, если нет места для блоков
    }
    size_t all_blocks = (all_free_disk / sizeof(TEMPFS_ENTITY)) - 1;                    /// Получаем количество доступных блоков информации
    return (all_blocks <= 0?0:all_blocks);                                              /// Возвращает кол-во блоков
}

/**
 * @brief Функция для проверка подписи
 */
int fs_tempfs_func_checkSign(uint16_t sign1, uint16_t sign2){
    return (sign1 + sign2 == 0x975f?1:0);                                   /// Проверяем подпись на основе сумме значений
}

void fs_tempfs_func_fixPackage(char* src, int count){
    for (int i = count; i < 9;i++){
		src[i] = 0;
	}
}

int fs_tempfs_tcache_update(const char Disk){
    tfs_log("[>] TCache update...\n");

    TEMPFS_Cache* __TCache__ = dpm_metadata_read(Disk);

    // dpm_metadata_write
    if (__TCache__ == 0){
        __TCache__ = malloc(sizeof(TEMPFS_Cache));
        if (__TCache__ == NULL){
            return 1;
        }
    }
    memset(__TCache__,  0, sizeof(TEMPFS_Cache));
    if (__TCache__->Boot != 0){
        tfs_log(" |--- Free boot (0x%x)...\n", __TCache__->Boot);
        free(__TCache__->Boot);
    }

    if (__TCache__->Files != 0){
        tfs_log(" |--- Free files (0x%x)...\n", __TCache__->Files);
        for (uint32_t i = 0; i < __TCache__->CountFiles; i++) {
            free(&__TCache__->Files[i]);
        }
        free(__TCache__->Files);
    }
    tfs_log(" |--- Malloc Boot...\n");
    __TCache__->Boot = malloc(sizeof(TEMPFS_BOOT));
    if (__TCache__->Boot == NULL){
        return 1;
    }
    tfs_log(" |  |--- Addr Boot: 0x%x\n", __TCache__->Boot);
    tfs_log(" |  |--- Zero Boot...\n");
    memset(__TCache__->Boot, 0, sizeof(TEMPFS_BOOT));
    tfs_log(" |  |--- Read Boot...\n");
    int read = dpm_read(Disk, 0, sizeof(TEMPFS_BOOT), __TCache__->Boot);
    if (read != sizeof(TEMPFS_BOOT)){
        tfs_log(" |       |--- Read: %d\n", read);
        return 2;
    }

    if (fs_tempfs_func_checkSign(__TCache__->Boot->Sign1, __TCache__->Boot->Sign2) != 1){
        tfs_log(" |      |--- Error check signature\n");
        return 3;
    }

    if (__TCache__->Boot->CountFiles > 0){
        __TCache__->Files = malloc(sizeof(TEMPFS_ENTITY) * __TCache__->Boot->CountFiles);
        if (__TCache__->Files == NULL){
            return 1;
        }
        memset(__TCache__->Files, 0, sizeof(TEMPFS_ENTITY) * __TCache__->Boot->CountFiles);
        size_t offset = 512;
        size_t inx = 0;
        tfs_log(" |--- [>] Enter while\n");
        while(1){
            if (inx + 1 > __TCache__->Boot->CountFiles) break;
            tfs_log(" |     |--- [>] %d | %d\n", inx + 1, __TCache__->Boot->CountFiles);
            int eread = dpm_read(Disk, offset, sizeof(TEMPFS_ENTITY), &__TCache__->Files[inx]);
            tfs_log(" |     |     |--- [>] Disk read\n");
            tfs_log(" |     |     |     |--- Offset: %d\n", offset);
            tfs_log(" |     |     |     |--- Size: %d\n", sizeof(TEMPFS_ENTITY));
            offset += sizeof(TEMPFS_ENTITY);                                                    /// Добавляем отступ для следующего поиска
            if (eread != sizeof(TEMPFS_ENTITY)){                                                /// Проверка на кол-во прочитаных байт
                tfs_log(" |      |    |--- Failed to load enough bytes for data.!\n");
                break;                                                                              /// Выходим с цикла, так как не удалось загрузить достаточно байт для данных.
            }
            if (__TCache__->Files[inx].Status != TEMPFS_ENTITY_STATUS_READY){
                tfs_log(" |           |--- No data.!\n");
                continue;                                                                           /// Пропускаем блок информации, тк тут нет информации
            }

            tfs_log(" |     |     |--- [>] File info\n");
            tfs_log(" |     |     |     |--- Name: %s\n", __TCache__->Files[inx].Name);
            tfs_log(" |     |     |     |--- Path: %s\n", __TCache__->Files[inx].Path);
            tfs_log(" |     |     |     |--- Size: %d\n", __TCache__->Files[inx].Size);
            tfs_log(" |     |     |     |--- Type: %s\n", (__TCache__->Files[inx].Type == TEMPFS_ENTITY_TYPE_FOLDER?"Folder":"File"));
            //tfs_log(" |     |     |     |--- Date: %s\n", Files[count].LastTime);
            tfs_log(" |     |     |     |--- CHMOD: 0x%x\n", __TCache__->Files[inx].CHMOD);
            tfs_log(" |     |     |--- Next!\n");
            inx++;
        }
    }
    __TCache__->CountFiles = __TCache__->Boot->CountFiles;
    __TCache__->BlocksAll = fs_tempfs_func_getCountAllBlocks(TMF_GETDISKSIZE(Disk));
    __TCache__->FreeAll = (__TCache__->BlocksAll - __TCache__->Boot->CountBlocks - __TCache__->Boot->CountFiles);
    __TCache__->Status = 1;

    dpm_metadata_write(Disk, (void*) __TCache__);
    return 0x7246;
}


TEMPFS_PACKAGE* fs_tempfs_func_readPackage(const char Disk, size_t Address){
    TEMPFS_PACKAGE* pack = malloc(sizeof(TEMPFS_PACKAGE));
    if (pack == NULL){
        return NULL;
    }
    memset(pack, 0, sizeof(TEMPFS_PACKAGE));

    int read = dpm_read(Disk, Address, sizeof(TEMPFS_PACKAGE), pack);
    return pack;
}

int fs_tempfs_func_writePackage(const char Disk, size_t Address, TEMPFS_PACKAGE* pack){
    int write = dpm_write(Disk, Address, sizeof(TEMPFS_PACKAGE), pack);
    return (write == sizeof(TEMPFS_PACKAGE)?1:0);
}

size_t fs_tempfs_func_getIndexEntity(const char Disk, char* Path){
    TEMPFS_Cache* __TCache__ = dpm_metadata_read(Disk);
    if (__TCache__ == 0 || __TCache__->Status != 1 || __TCache__->Boot->CountFiles <= 0){
        return -1;
    }
    char* dir = pathinfo(Path, PATHINFO_DIRNAME);                           /// Создаем данные о родительской папке
    char* basename = pathinfo(Path, PATHINFO_BASENAME);                     /// Создаем данные о базовом названии
    size_t offset = 512;
    size_t inx = 0;
    size_t ginx = 0;
    tfs_log(" |--- [>] Enter while\n");
    while(1){
        if (inx + 1 > __TCache__->Boot->CountFiles) break;
        tfs_log(" |     |--- [>] %d | %d\n", inx + 1, __TCache__->Boot->CountFiles);
        TEMPFS_ENTITY tmp = {0};
        int eread = dpm_read(Disk, offset, sizeof(TEMPFS_ENTITY), &tmp);
        tfs_log(" |     |     |--- [>] Disk read\n");
        tfs_log(" |     |     |     |--- Offset: %d\n", offset);
        tfs_log(" |     |     |     |--- Size: %d\n", sizeof(TEMPFS_ENTITY));
        offset += sizeof(TEMPFS_ENTITY);                                                    /// Добавляем отступ для следующего поиска
        if (eread != sizeof(TEMPFS_ENTITY)){                                                /// Проверка на кол-во прочитаных байт
            tfs_log(" |     |    |--- Failed to load enough bytes for data.!\n");
            break;                                                                              /// Выходим с цикла, так как не удалось загрузить достаточно байт для данных.
        }
        if (tmp.Status != TEMPFS_ENTITY_STATUS_READY){
            tfs_log(" |           |--- No data.!\n");
            ginx++;
            continue;                                                                           /// Пропускаем блок информации, тк тут нет информации
        }
        int is_in = strcmp(tmp.Path, dir);                       /// Провяем на совпадении пути
        int is_file = strcmp(tmp.Name, basename);                /// Провяем на совпадении имени

        if (is_in == 0 && is_file == 0){
            free(dir);
            free(basename);
            return ginx;
        }
        ginx++;
        inx++;
    }
    free(dir);
    free(basename);
    return -1;
}

TEMPFS_ENTITY* fs_tempfs_func_readEntity(const char Disk, char* Path){
    TEMPFS_ENTITY* entity = malloc(sizeof(TEMPFS_ENTITY));
    if (entity == NULL) return NULL;
    memset(entity, 0, sizeof(TEMPFS_ENTITY));
    TEMPFS_Cache* __TCache__ = dpm_metadata_read(Disk);
    if (__TCache__ == 0 || __TCache__->Status != 1 || __TCache__->CountFiles == 0){
        return entity;
    }

    char* dir = pathinfo(Path, PATHINFO_DIRNAME);                           /// Создаем данные о родительской папке
    char* basename = pathinfo(Path, PATHINFO_BASENAME);                     /// Создаем данные о базовом названии
    tfs_log("basename: %s | dir: %s\n",basename, dir);
    for(size_t cid = 0; cid < __TCache__->CountFiles; cid++){                         /// Поиск по циклу
        if (__TCache__->Files[cid].Status != 1){                                      /// Проверяем сущность на готовность
            continue;                                                               /// Пропускаем, так как сущность не готова
        }
        int is_in = strcmp(__TCache__->Files[cid].Path, dir);                       /// Провяем на совпадении пути
        int is_file = strcmp(__TCache__->Files[cid].Name, basename);                /// Провяем на совпадении имени

        tfs_log(" |- name: %s | path: %s\n",__TCache__->Files[cid].Name, __TCache__->Files[cid].Path);
        if (is_in == 0 && is_file == 0){                                       /// Проверка выполненых условий
            tfs_log("{^^^^ SELECTED}\n");
            memcpy(entity, &__TCache__->Files[cid], sizeof(TEMPFS_ENTITY));
            return entity;                                                               /// Элемент найден, возвращаем 1
        }
    }
    free(dir);                                                              /// Освобождение ОЗУ
    free(basename);                                                         /// Освобождение ОЗУ
    return entity;
}

int fs_tempfs_func_writeEntity(const char Disk, int Index, TEMPFS_ENTITY* entity){
    int write = dpm_write(Disk, 512 + (Index*sizeof(TEMPFS_ENTITY)), sizeof(TEMPFS_ENTITY), entity);
    return (write == sizeof(TEMPFS_ENTITY)?1:0);
}


int fs_tempfs_func_updateBoot(const char Disk, TEMPFS_BOOT* boot){
    int write = dpm_write(Disk, 0, sizeof(TEMPFS_BOOT), boot);
    return (write == sizeof(TEMPFS_BOOT)?1:0);
}

TEMPFS_BOOT* fs_tempfs_func_getBootInfo(const char Disk){
    TEMPFS_Cache* __TCache__ = dpm_metadata_read(Disk);
    if (__TCache__ == 0) return NULL;
    return __TCache__->Boot;
}

size_t fs_tempfs_func_getCountAllFreeEntity(const char Disk){
    TEMPFS_Cache* __TCache__ = dpm_metadata_read(Disk);
    if (__TCache__ == 0) return 0;
    return __TCache__->FreeAll;
}

int fs_tempfs_func_findFreePackage(const char Disk, int Skip){
    TEMPFS_Cache* __TCache__ = dpm_metadata_read(Disk);
    if (__TCache__ == 0 || __TCache__->Status != 1){                                                /// Получаем данные с кэша
        return -1;                                                               /// Кэш не готов к работе, возвращаем 0.
    }
    tfs_log("[>] Find free package...\n");

    TEMPFS_BOOT* boot = fs_tempfs_func_getBootInfo(Disk);                               /// Получаем данные о загрузочном блоке
    if (boot == NULL || fs_tempfs_func_checkSign(boot->Sign1, boot->Sign2) != 1) {      /// Проверка на NULL, а также проверка подписи
        tfs_log(" |--- [ERR] Signature error\n");
        return -1;                                                                         /// Проверка не удалась, возвращаем -1
    }


    int allcount = fs_tempfs_func_getCountAllBlocks(boot->EndDisk);                     /// Получаем ОБЩИЕ кол-во блоков
    if (allcount <= 0){                                                                 /// Если кол-во блоков меньше= нуля
        tfs_log(" |--- [ERR] No blocks\n");
        return -1;                                                                          /// Возвращаем -1
    }

    int adr = 0;
	for (int i = 0; i < allcount; i++){
		adr = (boot->EndDisk - sizeof(TEMPFS_PACKAGE) - (i * sizeof(TEMPFS_PACKAGE)));
		TEMPFS_PACKAGE* pack = fs_tempfs_func_readPackage(Disk, adr);

		tfs_log("[%d] Test PackAge\n", i);

		tfs_log(" |--- Addr           | %x\n",adr);
		tfs_log(" |--- Data           | %s\n",pack->Data);
		tfs_log(" |--- Status         | %d\n",pack->Status);
		tfs_log(" |--- Length         | %d\n",pack->Length);
		tfs_log(" |--- Next           | %x\n\n",pack->Next);

		if (pack->Status != TEMPFS_ENTITY_STATUS_READY && Skip !=0){
			Skip--;
			continue;
		}

		if (pack->Status != TEMPFS_ENTITY_STATUS_READY){
            free(pack);
            return adr;
		}
		free(pack);
	}

	return -1;
}

int fs_tempfs_func_findFreeInfoBlock(const char Disk){
    TEMPFS_Cache* __TCache__ = dpm_metadata_read(Disk);
    if (__TCache__ == 0) return 0;
    if (__TCache__->Status != 1){                                                /// Получаем данные с кэша
        return 0;                                                               /// Кэш не готов к работе, возвращаем 0.
    }
    size_t offset = 512;
    size_t cmx = 0;
    TEMPFS_ENTITY* tmp = malloc(sizeof(TEMPFS_ENTITY));                         /// Создаем временную сущность
    while(1){
        memset(tmp, 0, sizeof(TEMPFS_ENTITY));                                      /// Очищаем эту сущность
        int read = dpm_read(Disk, offset, sizeof(TEMPFS_ENTITY), tmp);              /// Загружаем данные с устройства
        if (read != sizeof(TEMPFS_ENTITY)) {                                        /// Проверяем количество загруженных байт
            free(tmp);
            return -1;                                                                  /// Возвращаем -1, так произошла ошибка.
        }
        if (tmp->Status == 0 || tmp->Type == 0){                                    /// Выполняем проверки
            free(tmp);                                                              /// Освобождение ОЗУ
            return cmx;                                                             /// Возвращаем индекс блока доступного для записи
        }
        cmx++;                                                                      /// Увеличиваем счетчик
        offset += sizeof(TEMPFS_ENTITY);                                            /// Увеличиваем смещение
    }
    free(tmp);                                                                  /// Освобождение ОЗУ
}


int fs_tempfs_func_findDIR(const char Disk, const char* Path){
    TEMPFS_Cache* __TCache__ = dpm_metadata_read(Disk);
    if (__TCache__ == 0 || __TCache__->Status != 1 || __TCache__->CountFiles == 0){/// Получаем данные с кэша
        printf("[>] %d == 0 || %d != 1 || %d == 0", __TCache__, __TCache__->Status, __TCache__->CountFiles);
        return 0;                                                               /// Кэш не готов к работе, возвращаем 0.
    }
    int ret = 0x00;
    char* dir = pathinfo(Path, PATHINFO_DIRNAME);                           /// Создаем данные о родительской папке
    char* basename = pathinfo(Path, PATHINFO_BASENAME);                     /// Создаем данные о базовом названии
    for(size_t cid = 0; cid < __TCache__->CountFiles; cid++){                         /// Поиск по циклу
        if (__TCache__->Files[cid].Status != 1){                                      /// Проверяем сущность на готовность
            continue;                                                               /// Пропускаем, так как сущность не готова
        }
        if (__TCache__->Files[cid].Type != 0x02){                                       /// Проверяем сущность на тип папки
            continue;                                                               /// Пропускаем, так как это не папка
        }
        int is_in = strcmp(__TCache__->Files[cid].Path, dir);                        /// Провяем на наличие родительской папки
        int is_ph = strcmp(__TCache__->Files[cid].Name, basename);                       /// Провяем на наличие самой папки
        if (is_in == 0){
            ret |= TEMPFS_DIR_INFO_ROOT;                                            /// Ставим флаг, что корневая папка найдена
        }
        if (is_ph == 0){
            ret |= TEMPFS_DIR_INFO_EXITS;                                           /// Ставим флаг, что основная папка найдена
        }
        if ((ret & TEMPFS_DIR_INFO_ROOT) && (ret & TEMPFS_DIR_INFO_EXITS)){     /// Проверка выполненых условий
            free(dir);                                                              /// Освобождение ОЗУ
            return ret;                                                             /// Элемент найден, возвращаем 1
        }
    }
    free(dir);                                                              /// Освобождение ОЗУ
    return ret;                                                                 /// Элемент не найден, возвращаем 0
}

size_t fs_tempfs_func_findFilesOnDIR(const char Disk, const char* Path){
    TEMPFS_Cache* __TCache__ = dpm_metadata_read(Disk);
    if (__TCache__ == 0 || __TCache__->Status != 1 || __TCache__->CountFiles == 0){
        return 0;
    }
    size_t ret = 0;
    for(size_t cid = 0; cid < __TCache__->CountFiles; cid++){
        if (__TCache__->Files[cid].Status != 1){
            continue;
        }
        int is_in = strcmp(__TCache__->Files[cid].Path, Path);
        if (is_in == 0){
            ret++;
        }
    }
    return ret;
}

int fs_tempfs_func_findFILE(const char Disk, const char* Path){
    TEMPFS_Cache* __TCache__ = dpm_metadata_read(Disk);
    if (__TCache__ == 0 || __TCache__->Status != 1 || __TCache__->CountFiles == 0){                /// Получаем данные с кэша
        return 0;                                                               /// Кэш не готов к работе, возвращаем 0.
    }

    char* dir = pathinfo(Path, PATHINFO_DIRNAME);                           /// Создаем данные о родительской папке
    char* basename = pathinfo(Path, PATHINFO_BASENAME);                     /// Создаем данные о базовом названии
    for(size_t cid = 0; cid < __TCache__->CountFiles; cid++){
        if (__TCache__->Files[cid].Status != 1){
            continue;                                                               /// Пропускаем, так как сущность не готова
        }
        if (__TCache__->Files[cid].Type != 0x01){
            continue;                                                               /// Пропускаем, так как это не файл
        }
        int is_in = fs_tempfs_func_findDIR(Disk, dir);
        int is_file = strcmp(__TCache__->Files[cid].Name, basename);                /// Провяем на совпадении имени
        //int is_in = strcmp(__TCache__->Files[cid].Path, dir);                       /// Провяем на совпадении пути
        if (is_in == 0x03 && is_file == 0){                                       /// Проверка выполненых условий
            free(dir);                                                              /// Освобождение ОЗУ
            free(basename);                                                         /// Освобождение ОЗУ
            return 1;                                                               /// Элемент найден, возвращаем 1
        }
    }
    free(dir);                                                              /// Освобождение ОЗУ
    free(basename);                                                         /// Освобождение ОЗУ
    return 0;                                                               /// Элемент не найден, возвращаем 0
}

int fs_tempfs_func_clearBlocks(const char Disk, size_t Addr){
    tfs_log("[!] Starting delete package: 0x%x\n", Addr);
    if (Addr == 0){
        tfs_log("[E] ERROR POINT\n");
        return 1;
    }
    TEMPFS_PACKAGE* pkg_free = malloc(sizeof(TEMPFS_PACKAGE));
    size_t ADRNOW = Addr;
    while(1){
        tfs_log(" |---[>] Starting get package: 0x%x\n", ADRNOW);
        TEMPFS_PACKAGE* pack = fs_tempfs_func_readPackage(Disk, ADRNOW);
        if (pack == NULL){
            tfs_log("      |--- NULL\n");
            free(pkg_free);
            return 0;
        }
        if (pack->Status == TEMPFS_ENTITY_STATUS_PKG_READY){
            tfs_log("      |--- Delete PKG to Address: 0x%x\n", ADRNOW);
            fs_tempfs_func_writePackage(Disk, ADRNOW, pkg_free);
        }
        if (pack->Next == -1){
            free(pack);
            break;
        }
        tfs_log("      |--- Next Address: 0x%x\n", pack->Next);
        ADRNOW = pack->Next;
        free(pack);
    }

    //fs_tempfs_func_writePackage(Disk, pkg_addr[i], pkg_free);
    tfs_log("[+] complete delete package\n");
    free(pkg_free);
    return 1;
}


void fs_tempfs_func_cacheUpdate(const char Disk){
    fs_tempfs_tcache_update(Disk);                                  /// Обновляем кэш устройства
}

size_t fs_tempfs_read(const char Disk, const char* Path, size_t Offset, size_t Size, void* Buffer){
    TEMPFS_ENTITY* ent = fs_tempfs_func_readEntity(Disk, Path);
    if (ent == NULL || ent->Status != 1 || ent->Point == 0){
        return 0;
    }

    char* Buf = (char*)malloc(Size);
    if (Buf == NULL){
        return 0;
    }
    memset(Buf, 0, Size);

    size_t total_bytes_read = 0;

    // Определяем пакет, с которого необходимо начать чтение на основе смещения
    TEMPFS_PACKAGE* pack = fs_tempfs_func_readPackage(Disk, ent->Point);
    while(pack != NULL && Offset > 0){
        // Пропускаем пакеты, пока смещение не уменьшится до 0
        size_t bytes_to_skip = (Offset < pack->Length) ? Offset : pack->Length;
        Offset -= bytes_to_skip;
        pack = (pack->Next != -1) ? fs_tempfs_func_readPackage(Disk, pack->Next) : NULL;
    }

    // Начинаем считывать данные с учетом смещения и количества
    while(pack != NULL && total_bytes_read < Size){
        size_t bytes_to_read = (Size - total_bytes_read < pack->Length) ? (Size - total_bytes_read) : pack->Length;
        memcpy(&Buf[total_bytes_read], pack->Data, bytes_to_read);
        total_bytes_read += bytes_to_read;

        pack = (pack->Next != -1) ? fs_tempfs_func_readPackage(Disk, pack->Next) : NULL;
    }

    // Копируем результат в указанный буфер
    memcpy(Buffer, Buf, total_bytes_read);

    // Освобождаем буфер
    free(Buf);

    return total_bytes_read;
}

size_t fs_tempfs_write(const char Disk, const char* Path, size_t Offset, size_t Size, void* Buffer){
    tfs_log("File write...\n");
    TEMPFS_ENTITY* ent = fs_tempfs_func_readEntity(Disk, Path);
    if (ent == NULL || ent->Status != TEMPFS_ENTITY_STATUS_READY) return 0;

    tfs_log("File next...\n");
    size_t src_size = ent->Size < Size + Offset ? Size + Offset : ent->Size;

    char* PREBUF = malloc(src_size);
    memset(PREBUF, 0, src_size);

    size_t fre = fs_tempfs_read(Disk, Path, 0, src_size, PREBUF);

    memcpy(PREBUF + Offset, Buffer, Size);

    tfs_log("PREBUF: \n%s\n", PREBUF);

    size_t src_seek = 0;
    size_t read = 0;
    char src_buf[500] = {0}; // Буфер для одного пакета

    TEMPFS_PACKAGE* pkg_free = malloc(sizeof(TEMPFS_PACKAGE));
    if (pkg_free == NULL){
        tfs_log("KMALLOC ERROR\n");
        return 0;
    }

    // Определяем количество пакетов, которые нужно записать
    size_t countPack = (src_size / 500) + 1; // Рассчитываем количество пакетов

    // Отчищаем старые блоки, если уже есть точка входа
    if (fs_tempfs_func_clearBlocks(Disk, ent->Point) == 0) {
        tfs_log("BLOCK CLEAR ERROR\n");
        return 0;
    }
    uint32_t* pkg_addr = malloc(sizeof(uint32_t) * countPack);

    for (size_t i = 0; i < countPack; i++) {
        // Находим свободный пакет для записи данных
        pkg_addr[i] = fs_tempfs_func_findFreePackage(Disk,i);
        if (pkg_addr[i] == -1) {
            tfs_log("NO FREE PACKAGE!!!\n");
            return 0;
        }
    }

    for (size_t i = 0; i < countPack; i++) {
        // Определяем размер данных для записи в текущем пакете
        size_t bytes_to_write = (src_size - src_seek >= 500) ? 500 : src_size - src_seek;

        // Копируем данные из буфера в текущий пакет
        memcpy(src_buf, (char*)PREBUF + src_seek, bytes_to_write);
        src_seek += bytes_to_write;

        // Записываем пакет в файловую систему
        pkg_free->Length = bytes_to_write;
        pkg_free->Next = (i < countPack - 1) ? fs_tempfs_func_findFreePackage(Disk, i + 1) : -1;
        pkg_free->Status = TEMPFS_ENTITY_STATUS_PKG_READY;
        memcpy(pkg_free->Data, src_buf, 500);
        fs_tempfs_func_writePackage(Disk, pkg_addr[i], pkg_free);

        // Сбрасываем буфер
        memset(src_buf, 0, 500);
    }

    tfs_log("Write complete!\n");

    // После записи данных обновляем информацию о файле
    size_t indexFile = fs_tempfs_func_getIndexEntity(Disk, Path);
    ent->Size = src_seek;
    ent->Point = pkg_addr[0];
    int went = fs_tempfs_func_writeEntity(Disk, indexFile, ent);
    free(ent);
    tfs_log("[>] Write entity (%d) to disk...\n", indexFile);


    // Обновляем информацию о загрузочном секторе
    TEMPFS_BOOT* boot = fs_tempfs_func_getBootInfo(Disk);
    if (boot == NULL || fs_tempfs_func_checkSign(boot->Sign1, boot->Sign2) != 1) {
        tfs_log(" |--- [ERR] TempFS signature did not match OR error reading TempFS boot sector\n");
        return 0;
    }

    // Увеличиваем общее количество блоков
    boot->CountBlocks += countPack;
    tfs_log("[>] Boot update data...\n");
    /// Пишем загрузочную
    int boot_write = fs_tempfs_func_updateBoot(Disk, boot);
    if (boot_write != 1){
        tfs_log(" |-- [ERR] An error occurred while writing the TempFS boot partition\n");
        return 0;
    }
    fs_tempfs_func_cacheUpdate(Disk);

    return src_seek;
}

FSM_FILE fs_tempfs_info(const char Disk, const char* Path){
    // fs_tempfs_func_readEntity
    FSM_FILE file = {0};
    TEMPFS_ENTITY* entity = fs_tempfs_func_readEntity(Disk, Path);
    if (entity == NULL || entity->Status != 1) return file;
    memcpy(file.Name, entity->Name, strlen(entity->Name));
    memcpy(file.Path, entity->Path, strlen(entity->Path));

    file.CHMOD = entity->CHMOD;
    file.Mode = entity->CHMOD;
    file.Ready = 1;
    file.Size = entity->Size;
    file.Type = (entity->Type == TEMPFS_ENTITY_TYPE_FOLDER?5:0);

    free(entity);
    return file;
}

FSM_DIR* fs_tempfs_dir(const char Disk, const char* Path){
    FSM_DIR* Dir = malloc(sizeof(FSM_DIR));
    memset(Dir, 0, sizeof(FSM_DIR));
    tfs_log("[>] Get DIR\n");
    TEMPFS_BOOT* boot = fs_tempfs_func_getBootInfo(Disk);
    if (boot == NULL || fs_tempfs_func_checkSign(boot->Sign1, boot->Sign2) != 1) {
        tfs_log(" |--- Error sign 0x%x %d %d\n",boot , boot->Sign1, boot->Sign2);
        return Dir;
    }
    FSM_FILE *Files = malloc(sizeof(FSM_FILE) * boot->CountFiles);
    if (Files == NULL) {
        tfs_log(" |--- Error malloc\n");
        return Dir;
    }
    size_t offset = 512;                                                                /// Óêàçûâàåì îòñòóï, ñ êîòîðîãî íà÷èíàåì ïîèñê
    size_t count = 0;                                                                   /// Òåêóùèå êîëè÷åñòâî íàéäåíûõ ñóùíîñòåé
    size_t CF = 0, CD = 0;                                                              /// Òåêóùèå êîëè÷åñòâî íàéäåíûõ (CF-ôàéëîâ|CD-ïàïîê)
    size_t CZ = 0;
    tfs_log(" |--- [>] Enter while\n");
    while(1){                                                                           /// Âõîäèì â öèêë äëÿ ïîèñêà ñóùíîñòåé
        if (CZ + 1 > boot->CountFiles){                                                  /// Ïðîâåðÿåì íà êîë-âî ïðîâåðåííûõ ñóùíîñòåé
            break;                                                                              /// Âûõîäèì ñ öèêëà, òàê êàê âñå äàííûå óæå íàéäåíû.
        }
        tfs_log(" |     |--- [>] %d > %d\n", CZ + 1, boot->CountFiles);
        TEMPFS_ENTITY* entity = malloc(sizeof(TEMPFS_ENTITY));                              /// Ñîçäàåì ñóùíîñòü
        if (entity == NULL) {                                                               /// Ïðîâåðÿåì óäàëîñü ëè âûäåëèòü ïàìÿòü, äëÿ äàííîé ñóùíîñòè
            break;                                                                              /// Âûõîäèì ñ öèêëà, òàê êàê íåò ÎÇÓ
        }
        int eread = dpm_read(Disk, offset, sizeof(TEMPFS_ENTITY), entity);                  /// Çàãðóæàåì äàííûå î ñóùíîñòè ñ äèñêà
        tfs_log(" |     |     |--- [>] Disk read\n");
        tfs_log(" |     |     |     |--- Offset: %d\n", offset);
        tfs_log(" |     |     |     |--- Size: %d\n", sizeof(TEMPFS_ENTITY));
        offset += sizeof(TEMPFS_ENTITY);                                                    /// Äîáàâëÿåì îòñòóï äëÿ ñëåäóþùåãî ïîèñêà
        if (eread != sizeof(TEMPFS_ENTITY)){                                                /// Ïðîâåðêà íà êîë-âî ïðî÷èòàíûõ áàéò
            tfs_log(" |      |    |--- Failed to load enough bytes for data.!\n");
            free(entity);                                                                       /// Îñâîáîæäàåì ÎÇÓ îò ñóùíîñòè
            break;                                                                              /// Âûõîäèì ñ öèêëà, òàê êàê íå óäàëîñü çàãðóçèòü äîñòàòî÷íî áàéò äëÿ äàííûõ.
        }
        if (entity->Status == TEMPFS_ENTITY_STATUS_ERROR ||                                 /// Ïðîâåðêà íà ñòàòóñ è òèï ñóùíîñòè
            entity->Type   == TEMPFS_ENTITY_TYPE_UNKNOWN){
            tfs_log(" |           |--- No data.!\n");
            free(entity);                                                                       /// Îñâîáîæäàåì ÎÇÓ îò ñóùíîñòè
            continue;                                                                           /// Ïðîïóñêàåì áëîê èíôîðìàöèè, òê òóò íåò èíôîðìàöèè
        }
        if ((strcmp(entity->Path, "/") == 0 && strcmp(entity->Name, "/") == 0) || strcmp(entity->Path, Path) != 0){
            free(entity);
            CZ++;                                                                      /// Îñâîáîæäàåì ÎÇÓ îò ñóùíîñòè
            continue;
        }
        Files[count].Ready = 1;                                                             /// Óêàçûâàåì ÷òî ñóùíîñòü åñòü
        Files[count].CHMOD = entity->CHMOD;                                                 /// Êîïèðóåì íàñòðîéêè CHMOD (RWES)
        Files[count].Size = entity->Size;                                                   /// Êîïèðóåì ðàçìåð ôàéëà
        memcpy(Files[count].Name, entity->Name, strlen(entity->Name));                      /// Êîïèðóåì èìÿ ôàéëà
        memcpy(Files[count].Path, entity->Path, strlen(entity->Path));                      /// Êîïèðóåì èìÿ ïóòè

        if (entity->Type == TEMPFS_ENTITY_TYPE_FOLDER){                                     /// Ïðîâåðÿåì ÿâëÿåòñÿ ëè ñóùíîñòü ïàïêîé?
            CD++;                                                                               /// Óâåëè÷èâàåì ñ÷åò÷èê ïàïîê
            Files[count].Type = 5;                                                              /// Óêàçûâàåì ÷òî ñóùíîñòü ïàïêà
        } else {                                                                            /// Åñëè ýòî íå ïàïêà, çíà÷èò ôàéë
            CF++;                                                                               /// Óâåëè÷èâàåì ñ÷åò÷èê ôàéëîâ
            Files[count].Type = 0;                                                              /// Óêàçûâàåì ÷òî ýòî ôàéë
        }
        tfs_log(" |     |     |--- [>] File info\n");
        tfs_log(" |     |     |     |--- Name: %s\n", Files[count].Name);
        tfs_log(" |     |     |     |--- Path: %s\n", Files[count].Path);
        tfs_log(" |     |     |     |--- Size: %d\n", Files[count].Size);
        tfs_log(" |     |     |     |--- Type: %s\n", (Files[count].Type == 5?"Folder":"File"));
        //tfs_log(" |     |     |     |--- Date: %s\n", Files[count].LastTime);
        tfs_log(" |     |     |     |--- CHMOD: 0x%x\n", Files[count].CHMOD);

        count++;
        CZ++;                                                                                 /// Óâåëè÷åâàåì ñ÷åò÷èê íàéäåííûõ ñóùíîñòåé
        free(entity);                                                                       /// Îñâîáîæäàåì ÎÇÓ îò ñóùíîñòè
        tfs_log(" |     |     |--- Next!\n");
    }

    Dir->Ready = 1;                                                                     /// Ñîîáùàåì ÷òî äàííûå çàãðóæåíû
	Dir->Count = count;                                                                 /// Çàïèñûâàåì êîë-âî íàéäåíûõ ñóùíîñòåé
	Dir->CountFiles = CF;                                                               /// Çàïèñûâàåì êîë-âî íàéäåíûõ ôàéëîâ
	Dir->CountDir = CD;
	Dir->CountOther = 0;
	Dir->Files = Files;

    return Dir;
}

int fs_tempfs_create(const char Disk,const char* Path,int Mode){
    tfs_log("[>] Creating a new entity\n");
    TEMPFS_ENTITY* entity = malloc(sizeof(TEMPFS_ENTITY));                  /// Создаем сущность
    memset(entity, 0, sizeof(TEMPFS_ENTITY));
    int find_dir = fs_tempfs_func_findDIR(Disk, Path);                      /// Ищем родительскую папку
    tfs_log(" |--- Folder search result: 0x%x\n", find_dir);
    if (Mode == 0)  {                                                       /// Создаем файл
        tfs_log(" |--- Creating a file\n");

        int find_file = fs_tempfs_func_findFILE(Disk, Path);                    /// Ищем такой файл в папке
        tfs_log(" |--- File search result: %d\n", find_file);
        if (find_file == 1){                                                    /// Проверка условий
            free(entity);                                                           /// Освобождение ОЗУ
            return 0;                                                               /// Возвращаем 0, так как такой файл уже существует
        }
        tfs_log(" |--- Filling in metadata\n");
        char* dir = pathinfo(Path, PATHINFO_DIRNAME);                           /// Создаем данные о родительской папке
        char* basename = pathinfo(Path, PATHINFO_BASENAME);                     /// Создаем данные о базовом названии
        memcpy(entity->Name, basename, strlen(basename));                       /// Копируем данные об имени файла
        memcpy(entity->Path, dir, strlen(dir));                                 /// Копируем данные об пути файла
        entity->CHMOD |= TEMPFS_CHMOD_READ | TEMPFS_CHMOD_WRITE;                /// Ставим биты, чтения и записи
        // entity->Date = 0;
        entity->Size = 0;                                                       /// Указываем размер файла
        entity->Status = TEMPFS_ENTITY_STATUS_READY;                            /// Указываем что файл готов к работе
        entity->Type = TEMPFS_ENTITY_TYPE_FILE;                                 /// Указываем что это файл
        tfs_log(" |--- Next step\n");
    } else {                                                                /// Создаем папку
        tfs_log(" |--- Creating a folder\n");
        tfs_log(" |--- %d | %d\n", (find_dir & TEMPFS_DIR_INFO_EXITS), !(find_dir & TEMPFS_DIR_INFO_ROOT));
        if ((find_dir & TEMPFS_DIR_INFO_EXITS) || !(find_dir & TEMPFS_DIR_INFO_ROOT)){
            free(entity);                                                           /// Освобождение ОЗУ
            return 0;                                                               /// Если родительская папка не существует, или существует основная папка, возвращаеим 0
        }
        char* dir = pathinfo(Path, PATHINFO_DIRNAME);
        char* basename = pathinfo(Path, PATHINFO_BASENAME);                     /// Создаем данные о базовом названии
        memcpy(entity->Name, basename, strlen(basename));                       /// Копируем данные об имени файла
        memcpy(entity->Path, dir, strlen(dir));                               /// Копируем данные об пути файла
        entity->CHMOD |= TEMPFS_CHMOD_READ | TEMPFS_CHMOD_WRITE;                /// Ставим биты, чтения и записи
        // entity->Date = 0;
        entity->Size = 0;                                                       /// Указываем размер файла
        entity->Status = TEMPFS_ENTITY_STATUS_READY;                            /// Указываем что файл готов к работе
        entity->Type = TEMPFS_ENTITY_TYPE_FOLDER;                               /// Указываем что это папка
        tfs_log(" |--- Next step\n");
    }
    tfs_log(" |--- Searching for a free block to record an entity\n");
    int inx = fs_tempfs_func_findFreeInfoBlock(Disk);                       /// Поиск свободного блока
    if (inx == -1){                                                         /// Проверка условий
        tfs_log(" |--- Couldn't find a free entity\n");
        free(entity);                                                           /// Освобождение ОЗУ
        return 0;                                                               /// Возвращаем 0, так как нет свободного блока
    }


    tfs_log(" |--- Writing data to the device\n");
    int wr_entity = fs_tempfs_func_writeEntity(Disk, inx, entity);
    free(entity);
    if (wr_entity != 1){
        tfs_log(" |-- [WARN] There was a problem when writed data on the 0x%x section\n", 512 + (inx * sizeof(TEMPFS_ENTITY)));
        return 0;
    }
    tfs_log(" |--- Updating boot sector\n");
    TEMPFS_BOOT* boot = fs_tempfs_func_getBootInfo(Disk);
    if (boot == NULL || fs_tempfs_func_checkSign(boot->Sign1, boot->Sign2) != 1) {
        tfs_log(" |--- [ERR] TempFS signature did not match OR error reading TempFS boot sector\n");
        return 0;
    }
    boot->CountFiles++;
    /// Пишем загрузочную
    int boot_write = fs_tempfs_func_updateBoot(Disk, boot);
    if (boot_write != 1){
        tfs_log(" |-- [ERR] An error occurred while writing the TempFS boot partition\n");
        return 0;
    }
    tfs_log(" |--- Updating the cache\n");
    fs_tempfs_func_cacheUpdate(Disk);
	return 1;
}

int fs_tempfs_delete(const char Disk,const char* Path,int Mode){
    TEMPFS_BOOT* boot = fs_tempfs_func_getBootInfo(Disk);
    if (boot == NULL || fs_tempfs_func_checkSign(boot->Sign1, boot->Sign2) != 1) {
        tfs_log(" |--- [ERR] TempFS signature did not match OR error reading TempFS boot sector\n");
        return 0;
    }

    size_t indexFile = fs_tempfs_func_getIndexEntity(Disk, Path);
    if (indexFile == -1){
        tfs_log("Element %s no found to delete...\n", Path);
        return 0;
    }
    TEMPFS_ENTITY* ent = malloc(sizeof(TEMPFS_ENTITY));
    if (ent == NULL){
        tfs_log("No malloc free entity...\n");
        return 0;
    }
    memset(ent, 0, sizeof(TEMPFS_ENTITY));
    TEMPFS_ENTITY* elem = fs_tempfs_func_readEntity(Disk, Path);
    if (elem == NULL){
        free(ent);
        return 0;
    }
    if ((elem->CHMOD & TEMPFS_CHMOD_SYS) || !(elem->CHMOD & TEMPFS_CHMOD_WRITE)){
        tfs_log("You don't have enough rights to delete this item!\n%s%s",((elem->CHMOD & TEMPFS_CHMOD_SYS)?"Reason: the file is a system file\n":""),(!(elem->CHMOD & TEMPFS_CHMOD_WRITE)?"Reason: no recording rights\n":""));
        return 0;
    }

    if (Mode == 0)  {
        tfs_log(" |--- Delete a file\n");
        size_t deleteblocks = fs_tempfs_func_clearBlocks(Disk, elem->Point);
        tfs_log(" |--- Delete blocks: %d\n", deleteblocks);
        boot->CountBlocks -= deleteblocks;
    } else {
        tfs_log(" |--- Delete a folder\n");
        size_t foundElems = fs_tempfs_func_findFilesOnDIR(Disk, Path);
        if (foundElems != 0){
            tfs_log(" |--- To delete a folder, you need to delete %d more items inside it.\n", foundElems);
            free(ent);
            free(elem);
            return 0;
        }
    }
    int wr_entity = fs_tempfs_func_writeEntity(Disk, indexFile, ent);
    tfs_log(" |--- Delete metadata file (%d)...", indexFile);
    free(ent);
    free(elem);

    boot->CountFiles--;
    /// Пишем загрузочную
    int boot_write = fs_tempfs_func_updateBoot(Disk, boot);
    if (boot_write != 1){
        tfs_log(" |-- [ERR] An error occurred while writing the TempFS boot partition\n");
        return 0;
    }
    tfs_log(" |--- Updating the cache\n");

    fs_tempfs_func_cacheUpdate(Disk);
	return 1;
}

void fs_tempfs_label(const char Disk, char* Label){
    TEMPFS_BOOT* boot = fs_tempfs_func_getBootInfo(Disk);
    if (boot == NULL || fs_tempfs_func_checkSign(boot->Sign1, boot->Sign2) != 1) {
        tfs_log(" |--- [ERR] TempFS signature did not match OR error reading TempFS boot sector\n");
        return;
    }
	memcpy(Label,boot->Label,strlen(boot->Label));
}

int fs_tempfs_detect(const char Disk){
    tfs_log("\n[>] Attempt to check the boot sector\n");
    int ret = fs_tempfs_tcache_update(Disk);
    if (ret != 0x7246){
        tfs_log(" |--- [>] Return code: 0x%x\n", ret);
        return 0;
    }
	return 1;
}

void fs_tempfs_format(const char Disk){
    tfs_log("\n[>] Formatting for TempFS has started...\n");
    /// Создаем данные для записи
    TEMPFS_BOOT* boot = malloc(sizeof(TEMPFS_BOOT));
    TEMPFS_ENTITY* tmp = malloc(sizeof(TEMPFS_ENTITY));
    /// Заполняем данные нулями
    memset(boot, 0, sizeof(TEMPFS_BOOT));
    memset(tmp, 0, sizeof(TEMPFS_ENTITY));

    /// Заполняем базовую информацию
    boot->Sign1 = 0x7246;
    boot->Sign2 = 0x2519;
    memcpy(boot->Label,"New disk",strlen("New disk"));
    boot->EndDisk = TMF_GETDISKSIZE(Disk);
    boot->CountBlocks = 0;
    boot->CountFiles = 1;

    /// Пишем загрузочную
    int write = fs_tempfs_func_updateBoot(Disk, boot);
    if (write != 1){
        tfs_log(" |-- [ERR] An error occurred while writing the TempFS boot partition\n");
        return;
    }

    /// Выполняем расчеты о свободном пространстве диске
    size_t all_free_disk = (boot->EndDisk) - (sizeof(TEMPFS_BOOT)) - 1;
    if (all_free_disk <= 0){
        tfs_log(" |-- [ERR] The file system requires a minimum of 1024 bytes of memory\n");
        tfs_log(" |-- %d = (%d - %d - 1)\n", all_free_disk, boot->EndDisk, (sizeof(TEMPFS_BOOT)));
        tfs_log(" |-- INTERRUPTED!!");
        return;
    }
    /// Получаем количество доступных блоков информации
    size_t all_blocks = (all_free_disk / sizeof(TEMPFS_ENTITY)) - 1;
    if (all_blocks <= 0){
        tfs_log(" |-- [WARN] There are no free blocks left for file system elements!\n");
        all_blocks = 0;
    }

    /// Затираем все данные с диска
    for (size_t abx = 0; abx < all_blocks; abx++){
        tfs_log(" |-- [>] [%d | %d] Clearing the hard drive of old data\n",abx + 1,all_blocks);
        int wr_entity = fs_tempfs_func_writeEntity(Disk, abx, tmp);
        if (wr_entity != 1){
            tfs_log(" |-- [WARN] There was a problem when erasing data on the 0x%x section\n", 512 + (abx * sizeof(TEMPFS_ENTITY)));
        }
    }

    /// Создаем корневую папку
    tmp->CHMOD |= TEMPFS_CHMOD_READ | TEMPFS_CHMOD_WRITE | TEMPFS_CHMOD_SYS; /// Ставим биты, чтения, записи и системы
    tmp->Status = TEMPFS_ENTITY_STATUS_READY;
    tmp->Type   = TEMPFS_ENTITY_TYPE_FOLDER;
    memcpy(tmp->Name, "/", strlen("/"));
    memcpy(tmp->Path, "/", strlen("/"));
    memcpy(tmp->Owner, "root", strlen("root"));

    int root_write = fs_tempfs_func_writeEntity(Disk, 0, tmp);
    if (root_write != 1){
        tfs_log(" |-- [WARN] Failed to write the root directory\n");
    }
    tfs_log(" |-- Disk formatting is complete!\n");
    tfs_log(" |-- Label: %s\n", boot->Label);
    tfs_log(" |-- Free space: %d\n", all_free_disk);
    tfs_log(" |-- Free blocks: %d\n", all_blocks);

    /// Освобождаем оперативную память
    free(tmp);
    free(boot);
}
