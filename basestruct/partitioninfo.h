#ifndef PARTITIONINFO_H
#define PARTITIONINFO_H
#include "commondef.h"
#include <QString>

class QDBusArgument;
class PartitionInfo
{
public:
    PartitionInfo();
    Sector get_sector_length() const;
    Byte_Value get_byte_length() const;
    bool operator==(const PartitionInfo &info) const;

public:
    QString device_path;
    int partition_number;
    int type; //PartitionType: UNALLOCATED, PRIMARY, LOGICAL, etc...
    int status; //PartitionStatus: STAT_REAL, STAT_NEW, etc..
    int alignment; //PartitionAlignment: ALIGN_CYLINDER, ALIGN_STRICT, etc
    int fstype;
    QString uuid;
    QString name;
    Sector sector_start;
    Sector sector_end;
    Sector sectors_used;
    Sector sectors_unused;
    Sector sectors_unallocated; //Difference between the size of the partition and the file system
    Sector significant_threshold; //Threshold from intrinsic to significant unallocated sectors
    Sector free_space_before; //Free space preceding partition value
    Byte_Value sector_size; //Sector size of the disk device needed for converting to/from sectors and bytes.
    Byte_Value fs_block_size; // Block size of of the file system, or -1 when unknown.
    QString path; //partition path
    QString filesystem_label;
    bool inside_extended;
    bool busy;
    bool fs_readonly;
    QVector<QString> mountpoints;
};
Q_DECLARE_METATYPE(PartitionInfo)
QDBusArgument &operator<<(QDBusArgument &argument, const PartitionInfo &info);
const QDBusArgument &operator>>(const QDBusArgument &argument, PartitionInfo &info);

typedef QVector<PartitionInfo> PartitionVec;
Q_DECLARE_METATYPE(PartitionVec)

#endif // PARTITIONINFO_H
