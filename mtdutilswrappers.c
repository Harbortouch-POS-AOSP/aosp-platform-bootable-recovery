#include "mtdutilswrappers.h"

#include "common.h"
#include "mtdutils/mounts.c"
#include "mtdutils/mtdutils.c"

int LOGUI_mtd_scan_partitions()
{
   LOGUI("MTD scanning partitions...");
   return mtd_scan_partitions();
}

const MtdPartition* LOGUI_mtd_find_partition_by_name(const char *name)
{
   LOGUI("MTD finding partition %s by name...\n", name);
   return mtd_find_partition_by_name(name);
}

int LOGUI_mtd_mount_partition(const MtdPartition *partition,
   const char *mount_point,
   const char *filesystem, int read_only)
{
   LOGUI("MTD mount partition %s...\n", partition->name);
   return mtd_mount_partition(partition, mount_point,
                              filesystem, read_only);
}

int LOGUI_scan_mounted_volumes()
{
    LOGUI("Scanning mounted volumes...\n");
    return scan_mounted_volumes();
}

const MountedVolume* LOGUI_find_mounted_volume_by_device(const char *device)
{
    LOGUI("Find mounted volume by device %s...\n", device);
    return find_mounted_volume_by_device(device);
}

const MountedVolume* LOGUI_find_mounted_volume_by_mount_point(const char *mount_point)
{
    return find_mounted_volume_by_mount_point(mount_point);
}

int LOGUI_unmount_mounted_volume(const MountedVolume *volume)
{
   LOGUI("Unmount mounted volume from mount point %s...\n", volume->mount_point);
   return unmount_mounted_volume(volume);
}

int LOGUI_remount_read_only(const MountedVolume* volume)
{
    LOGUI("Remount read only device by mount path %s...\n", volume->mount_point);
    return remount_read_only(volume);
}

void LOGUI_free_volume_internals(const MountedVolume *volume, int zero)
{
    LOGUI("Free volume %s internals...\n", volume->mount_point);
    free_volume_internals(volume, zero);
}