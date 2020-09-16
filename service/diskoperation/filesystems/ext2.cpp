/**
 * @copyright 2020-2020 Uniontech Technology Co., Ltd.
 *
 * @file ext2.cpp
 *
 * @brief 文件格式ext操作类
 *
 * @date 2020-09-15 13:51
 *
 * Author: liweigang  <liweigang@uniontech.com>
 *
 * Maintainer: liweigang  <liweigang@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ext2.h"
#include "utils.h"

#include <QDebug>

namespace DiskManager {

EXT2::EXT2(FSType type)
    : m_forceAuto64bit(false)
    , m_specificType(type)
{
}

FS EXT2::getFilesystemSupport()
{
    FS fs(m_specificType);

    fs.busy = FS::GPARTED;
    QString output, error;
    m_mkfsCmd = "mkfs." + Utils::FSTypeToString(m_specificType);
    bool have_64bit_feature = false;
    if (!Utils::find_program_in_path(m_mkfsCmd).isEmpty()) {
        fs.create = FS::EXTERNAL;
        fs.create_with_label = FS::EXTERNAL;

        // Determine mkfs.ext4 version specific capabilities.
        m_forceAuto64bit = false;
        if (m_specificType == FS_EXT4) {
            Utils::executcmd(QString("%1%2").arg(m_mkfsCmd).arg(" -V"), output, error);
            int mke2fs_major_ver = 0;
            int mke2fs_minor_ver = 0;
            int mke2fs_patch_ver = 0;
            if (sscanf(output.toLatin1().data(), "mke2fs %d.%d.%d",
                       &mke2fs_major_ver, &mke2fs_minor_ver, &mke2fs_patch_ver)
                    >= 2) {
                // Ext4 64bit feature was added in e2fsprogs 1.42, but
                // only enable large volumes from 1.42.9 when a large
                // number of 64bit bugs were fixed.
                // *   Release notes, E2fsprogs 1.42 (November 29, 2011)
                //     http://e2fsprogs.sourceforge.net/e2fsprogs-release.html#1.42
                // *   Release notes, E2fsprogs 1.42.9 (December 28, 2013)
                //     http://e2fsprogs.sourceforge.net/e2fsprogs-release.html#1.42.9
                have_64bit_feature = (mke2fs_major_ver > 1)
                                     || (mke2fs_major_ver == 1 && mke2fs_minor_ver > 42)
                                     || (mke2fs_major_ver == 1 && mke2fs_minor_ver == 42 && mke2fs_patch_ver >= 9);

                // (#766910) E2fsprogs 1.43 creates 64bit ext4 file
                // systems by default.  RHEL/CentOS 7 configured e2fsprogs
                // 1.42.9 to create 64bit ext4 file systems by default.
                // Theoretically this can be done when 64bit feature was
                // added in e2fsprogs 1.42.  GParted will re-implement the
                // removed mke2fs.conf(5) auto_64-bit_support option to
                // avoid the issues with multiple boot loaders not working
                // with 64bit ext4 file systems.
                m_forceAuto64bit = (mke2fs_major_ver > 1)
                                   || (mke2fs_major_ver == 1 && mke2fs_minor_ver >= 42);
            }
        }
    }

    if (!Utils::find_program_in_path("dumpe2fs").isEmpty()) {
        fs.read = FS::EXTERNAL;
        fs.online_read = FS::EXTERNAL;
    }

    if (!Utils::find_program_in_path("tune2fs").isEmpty()) {
        fs.read_uuid = FS::EXTERNAL;
        fs.write_uuid = FS::EXTERNAL;
    }

    if (!Utils::find_program_in_path("e2label").isEmpty()) {
        fs.read_label = FS::EXTERNAL;
        fs.write_label = FS::EXTERNAL;
    }

    if (!Utils::find_program_in_path("e2fsck").isEmpty())
        fs.check = FS::EXTERNAL;

    if (!Utils::find_program_in_path("resize2fs").isEmpty()) {
        fs.grow = FS::EXTERNAL;

        if (fs.read) // Needed to determine a min file system size..
            fs.shrink = FS::EXTERNAL;
    }

    if (fs.check) {
        fs.copy = fs.move = FS::GPARTED;

        // If supported, use e2image to copy/move the file system as it only
        // copies used blocks, skipping unused blocks.  This is more efficient
        // than copying all blocks used by GParted's internal method.
        if (!Utils::find_program_in_path("e2image").isEmpty()) {
            Utils::executcmd("e2image", output, error);
            if (Utils::regexp_label(output, "(-o src_offset)") == "-o src_offset")
                fs.copy = fs.move = FS::EXTERNAL;
        }
    }
    // Maximum size of an ext2/3/4 volume is 2^32 - 1 blocks, except for ext4 with
    // 64bit feature.  That is just under 16 TiB with a 4K block size.
    // *   Ext4 Disk Layout, Blocks
    //     https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Blocks
    // FIXME: Rounding down to whole MiB here should not be necessary.  The Copy, New
    // and Resize/Move dialogs should limit FS correctly without this.  See bug
    // #766910 comment #12 onwards for further discussion.
    //     https://bugzilla.gnome.org/show_bug.cgi?id=766910#c12
    if (m_specificType == FS_EXT2 || m_specificType == FS_EXT3 || (m_specificType == FS_EXT4 && !have_64bit_feature))
        m_fsLimits.max_size = Utils::floor_size(16 * TEBIBYTE - 4 * KIBIBYTE, MEBIBYTE);

    return fs;
}

void EXT2::setUsedSectors(Partition &partition)
{
    QString output, error, strmatch, strcmd;
    m_blocksSize = m_numOfFreeOrUsedBlocks = m_totalNumOfBlock = -1;
    strcmd = QString("dumpe2fs -h %1").arg(partition.getPath());
    if (!Utils::executcmd(strcmd, output, error)) {
        strmatch = ("Block count:");
        int index = output.indexOf(strmatch);
        if (index >= 0 && index < output.length()) {
            m_totalNumOfBlock = Utils::regexp_label(output, QString("(?<=Block count:).*(?=\n)")).trimmed().toLong();
        }
//        qDebug() << output;
        qDebug() << output.mid(index, strmatch.length()).toLatin1() << m_totalNumOfBlock;
        strmatch = ("Block size:");
        index = output.indexOf(strmatch);
        if (index >= 0 && index < output.length()) {
            m_blocksSize = Utils::regexp_label(output, QString("(?<=Block size:).*(?=\n)")).trimmed().toLong();
        }
        qDebug() << output << output.mid(index, strmatch.length()).toLatin1() << m_blocksSize;

        if (partition.m_busy) {
            Byte_Value fs_all;
            Byte_Value fs_free;
            if (Utils::get_mounted_filesystem_usage(partition.getMountPoint(), fs_all, fs_free) == 0) {
                partition.setSectorUsage(qRound64(fs_all / double(partition.m_sectorSize)), qRound64(fs_free / double(partition.m_sectorSize)));
                partition.m_fsBlockSize = m_blocksSize;
            }
        } else {
            // Resize2fs won't shrink a file system smaller than it's own
            // estimated minimum size, so use that to derive the free space.
            m_numOfFreeOrUsedBlocks = -1;
            if (!Utils::executcmd(QString("resize2fs -P %1").arg(partition.getPath()), output, error)) {
                if (sscanf(output.toLatin1(), "Estimated minimum size of the filesystem: %lld", &m_numOfFreeOrUsedBlocks) == 1)
                    m_numOfFreeOrUsedBlocks = m_totalNumOfBlock - m_numOfFreeOrUsedBlocks;
            }
            // Resize2fs can fail reporting please run fsck first.  Fall back
            // to reading dumpe2fs output for free space.
            if (m_numOfFreeOrUsedBlocks == -1) {
                strmatch = "Free blocks:";
                index = output.indexOf(strmatch);
                if (index < output.length())
                    sscanf(output.mid(index, strmatch.length()).toLatin1(), "Free blocks: %lld", &m_numOfFreeOrUsedBlocks);
            }

            if (m_totalNumOfBlock > -1 && m_numOfFreeOrUsedBlocks > -1 && m_blocksSize > -1) {
                m_totalNumOfBlock = qRound64(m_totalNumOfBlock * (m_blocksSize / double(partition.m_sectorSize)));
                m_numOfFreeOrUsedBlocks = qRound64(m_numOfFreeOrUsedBlocks * (m_blocksSize / double(partition.m_sectorSize)));
                qDebug() << "111111111111111111111111" << m_totalNumOfBlock << m_numOfFreeOrUsedBlocks << m_blocksSize;
                partition.setSectorUsage(m_totalNumOfBlock, m_numOfFreeOrUsedBlocks);
                partition.m_fsBlockSize = m_blocksSize;
            }
        }


    } else {
        qDebug() << __FUNCTION__ << "dumpe2fs -h failed :" << output << error;
    }
}

void EXT2::readLabel(Partition &partition)
{
    QString output, error;
    if (!Utils::executcmd(QString("e2label %1").arg(partition.getPath()), output, error)) {
        partition.setFilesystemLabel(output.trimmed());
    }
    qDebug() << __FUNCTION__ << output << error;
}

bool EXT2::writeLabel(const Partition &partition)
{
    QString output, error;
    int exitcode = Utils::executcmd(QString("e2label %1 %2").arg(partition.getPath()).arg(partition.getFileSystemLabel()), output, error);
    qDebug() << __FUNCTION__ << output << error;
    return exitcode == 0;
}

void EXT2::readUuid(Partition &partition)
{
    QString output, error;
    if (!Utils::executcmd(QString("tune2fs -l %1").arg(partition.getPath()), output, error)) {
        partition.m_uuid = Utils::regexp_label(output, "(?<=Filesystem UUID:).*(?=\n)").trimmed();
    }
    qDebug() << __FUNCTION__ << output << error;
}

bool EXT2::writeUuid(const Partition &partition)
{
    QString output, error;
    int exitcode = Utils::executcmd(QString("tune2fs -U random ").arg(partition.getPath()), output, error);
    return exitcode == 0 || error.compare("Unknown error") == 0;
}

bool EXT2::create(const Partition &new_partition)
{
    QString features, output, error, cmd;
    if (m_forceAuto64bit) {
        // (#766910) Manually implement mke2fs.conf(5) auto_64-bit_support option
        // by setting or clearing the 64bit feature on the command line depending
        // of the partition size.
        if (new_partition.getByteLength() >= 16 * TEBIBYTE)
            features = " -O 64bit";
        else
            features = " -O ^64bit";
    }
    QString strlabel = new_partition.getFileSystemLabel();
    if(strlabel == " ") {
      cmd = QString("%1%2%3%4%5").arg(m_mkfsCmd).arg(" -F").arg(features).arg(" ").arg(new_partition.getPath());
    }
    else {
        strlabel = strlabel.isEmpty() ? strlabel : QString(" -L %1").arg(strlabel);
        cmd = QString("%1%2%3%4%5%6").arg(m_mkfsCmd).arg(" -F").arg(features).arg(strlabel).arg(" ").arg(new_partition.getPath());
    }
    qDebug() << " EXT2::create***** " << cmd;
    int exitcode = Utils::executcmd(cmd, output, error);
    qDebug() << "EXT2::create-------" << output << error;
    return exitcode == 0 || error.compare("Unknown error") == 0;
}

bool EXT2::resize(const Partition &partitionNew, bool fillPartition)
{
    QString str_temp = QString("resize2fs -p %1").arg(partitionNew.getPath());

    if (!fillPartition) {
        str_temp.append(qFloor(Utils::sector_to_unit(partitionNew.getSectorLength(), partitionNew.m_sectorSize, UNIT_KIB)));
        str_temp.append("K");
    }
    QString output, error;
    int exitcode = Utils::executcmd(str_temp, output, error);
    return exitcode == 0 || error.compare("Unknown error") == 0;
}

bool EXT2::checkRepair(const Partition &partition)
{
    QString output, error;
    int exitcode = Utils::executcmd(QString("e2fsck -f -y -v -C 0 %1").arg(partition.getPath()), output, error);
    qDebug() << QString("EXT2::check_repair---%1----%2").arg(output).arg(error);
    return exitcode == 0 || error.compare("Unknown error") == 0;
}

} // namespace DiskManager
