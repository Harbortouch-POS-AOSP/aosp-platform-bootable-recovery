/**
 * Wrapper functions upon minzip library to show detail log info to UI cpp project
 * only show additional info about executed function using LOGUI() macros
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "minzip/Hash.h"
#include "minzip/SysUtil.h"
#include "minzip/Zip.h"

HashTable* LOGUI_mzHashTableCreate(size_t initialSize, HashFreeFunc freeFunc);

void LOGUI_mzHashTableFree(HashTable* pHashTable);

int LOGUI_sysMapFile(const char* fn, MemMapping* pMap);

void LOGUI_sysReleaseMap(MemMapping* pMap);

int LOGUI_mzOpenZipArchive(unsigned char* addr, size_t length, ZipArchive* pArchive);

const ZipEntry* LOGUI_mzFindZipEntry(const ZipArchive* pArchive, const char* entryName);

bool LOGUI_mzProcessZipEntryContents(const ZipArchive *pArchive,
   const ZipEntry *pEntry, ProcessZipEntryContentsFunction processFunction,
   void *cookie);

bool LOGUI_mzExtractZipEntryToFile(const ZipArchive *pArchive,
    const ZipEntry *pEntry, int fd);

void LOGUI_mzCloseZipArchive(ZipArchive* pArchive);

#ifdef __cplusplus
}
#endif