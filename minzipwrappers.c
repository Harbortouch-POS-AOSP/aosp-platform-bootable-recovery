#include "minzipwrappers.h"

#include "common.h"

HashTable* LOGUI_mzHashTableCreate(size_t initialSize, HashFreeFunc freeFunc)
{
    LOGUI("MZ create hash table\n");
    return mzHashTableCreate(initialSize, freeFunc);
}

void LOGUI_mzHashTableFree(HashTable* pHashTable)
{
    LOGUI("MZ free hash table\n");
    mzHashTableFree(pHashTable);
}

int LOGUI_sysMapFile(const char* fn, MemMapping* pMap)
{
    LOGUI("System call for map file %s\n", fn);
    return sysMapFile(fn, pMap);
}

void LOGUI_sysReleaseMap(MemMapping* pMap)
{
    LOGUI("System call release map file\n");
    sysReleaseMap(pMap);
}

int LOGUI_mzOpenZipArchive(unsigned char* addr, size_t length, ZipArchive* pArchive)
{
    LOGUI("Opening zip archive...\n");
    return mzOpenZipArchive(addr, length, pArchive);
}

const ZipEntry* LOGUI_mzFindZipEntry(const ZipArchive* pArchive, const char* entryName)
{
    LOGUI("MZ finding zip entry...\n");
    return mzFindZipEntry(pArchive, entryName);
}

bool LOGUI_mzProcessZipEntryContents(const ZipArchive *pArchive,
   const ZipEntry *pEntry, ProcessZipEntryContentsFunction processFunction,
   void *cookie)
{
    LOGUI("MZ process zip entry contents...\n");
    return mzProcessZipEntryContents(pArchive, pEntry, processFunction, cookie);
}

bool LOGUI_mzExtractZipEntryToFile(const ZipArchive *pArchive,
    const ZipEntry *pEntry, int fd)
{
    LOGUI("MZ extract zip entry to file...\n");
    return mzExtractZipEntryToFile(pArchive, pEntry, fd);
}

void LOGUI_mzCloseZipArchive(ZipArchive* pArchive)
{
    LOGUI("MZ close zip archive...\n");
    mzCloseZipArchive(pArchive);
}