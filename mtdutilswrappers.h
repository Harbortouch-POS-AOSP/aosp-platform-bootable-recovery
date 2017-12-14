/**
 * Wrapper functions upon mtdutils library to show detail log info to UI cpp project
 * only show additional info about executed function using LOGUI() macros
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "mtdutils/mounts.h"
#include "mtdutils/mtdutils.h"

int LOGUI_mtd_scan_partitions();

const MtdPartition* LOGUI_mtd_find_partition_by_name(const char *name);

int LOGUI_mtd_mount_partition(const MtdPartition *partition,
   const char *mount_point,
   const char *filesystem, int read_only);

int LOGUI_scan_mounted_volumes();

const MountedVolume* LOGUI_find_mounted_volume_by_device(const char *device);

const MountedVolume* LOGUI_find_mounted_volume_by_mount_point(const char *mount_point);

int LOGUI_unmount_mounted_volume(const MountedVolume *volume);

int LOGUI_remount_read_only(const MountedVolume* volume);

static inline void LOGUI_free_volume_internals(const MountedVolume *volume, int zero);

#ifdef __cplusplus
}
#endif