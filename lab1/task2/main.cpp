#include <algorithm>
#include <iostream>
#include <ostream>
#include <sys/utsname.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <map>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <mntent.h>
#include <sys/statvfs.h>
#include "utils/StringUtils.h"

utsname Uname()
{
    struct utsname utsname{};

    if (uname(&utsname) < 0)
    {
        throw std::runtime_error(std::strerror(errno));
    }

    return utsname;
}

std::string Exec(const std::string& cmd)
{
    std::string result;
    FILE* file = popen(cmd.c_str(), "r");

    if (file == nullptr)
    {
        throw std::runtime_error(std::strerror(errno));
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), file) != nullptr)
    {
        result += buffer;
    }

    pclose(file);
    return result;
}

std::string GetOSVersion()
{
    const auto rawVersionData = Exec("lsb_release --description");
    auto keyValuePair = Separate(rawVersionData, ':');
    if (keyValuePair.size() != 2)
    {
        throw std::runtime_error("[GetOSVersion] Error while parsing version info.");
    }
    return keyValuePair[1];
}

std::string GetUserLogin()
{
    return getlogin();
}

constexpr auto MAX_COMPATIBLE_HOST_NAME_LENGTH = 255;
std::string GetHostname()
{
    char hostname[MAX_COMPATIBLE_HOST_NAME_LENGTH];
    gethostname(hostname, MAX_COMPATIBLE_HOST_NAME_LENGTH);

    return hostname;
}

std::map<std::string, unsigned long> GetRandomAccessMemoryInfo()
{
    std::map<std::string, unsigned long> result;
    std::ifstream memoryInfoFile("/proc/meminfo", std::ios::in);

    std::string key;
    unsigned long value;
    std::string unit;
    while (memoryInfoFile >> key >> value >> unit)
    {
        Trim(key);
        if (!key.empty() && key.back() == ':')
        {
            key.pop_back();
            Trim(key);
        }

        result[key] = value;
    }

    return result;
}

std::array<double, 3> GetLoadAverage()
{
    struct sysinfo sysPerformanceInfo{};
    if (sysinfo(&sysPerformanceInfo) < 0)
    {
        throw std::runtime_error(std::strerror(errno));
    };

    return {
        static_cast<double>(sysPerformanceInfo.loads[0]) / 65536.0,
        static_cast<double>(sysPerformanceInfo.loads[1]) / 65536.0,
        static_cast<double>(sysPerformanceInfo.loads[2]) / 65536.0,
    };
}

struct MountInfo
{
    std::string device;
    std::string mountPoint;
    std::string fsType;
    std::string options;

    unsigned long long totalBytes{};
    unsigned long long freeBytes{};
};

void GetStatsFromVFS(MountInfo& info)
{
    struct statvfs buff{};
    if (statvfs(info.mountPoint.c_str(), &buff) == 0)
    {
        const auto blockSize = buff.f_bsize;

        info.totalBytes = buff.f_blocks * blockSize;
        info.freeBytes = buff.f_bfree * blockSize;
    }
}

std::vector<MountInfo> GetAllMounts()
{
    std::vector<MountInfo> mounts;
    FILE* mountsTab = setmntent("/proc/mounts", "r");

    if (mountsTab == nullptr)
    {
        throw std::runtime_error("Can't open /proc/mounts");
    }

    struct mntent* entry;
    while ((entry = getmntent(mountsTab)) != nullptr)
    {
        MountInfo info;
        info.device = entry->mnt_fsname;
        info.mountPoint = entry->mnt_dir;
        info.fsType = entry->mnt_type;
        info.options = entry->mnt_opts;
        GetStatsFromVFS(info);

        mounts.push_back(info);
    }

    endmntent(mountsTab);
    std::ranges::sort(
        mounts,
        [](const MountInfo& a, const MountInfo& b){return a.mountPoint.size() < b.mountPoint.size();}
    );
    return mounts;
}

int main()
{
    const auto sysInfo = Uname();
    auto ramInfo = GetRandomAccessMemoryInfo();
    const auto loadAverage = GetLoadAverage();
    const auto mounts = GetAllMounts();

    std::cout << "OS: " << GetOSVersion() << std::endl;
    std::cout << "Kernel: " << sysInfo.sysname << " " << sysInfo.release << std::endl;
    std::cout << "Architecture: " << sysInfo.machine << std::endl;
    std::cout << "Hostname: " << sysInfo.nodename << std::endl;
    std::cout << "User: " << GetUserLogin() << std::endl;
    std::cout << "RAM: " << ramInfo["MemFree"] / 1024 << "MB free / "
        << ramInfo["MemTotal"] / 1024 << "MB total" << std::endl;
    std::cout << "Swap: " << ramInfo["SwapFree"] / 1024 << "MB free / "
        << ramInfo["SwapTotal"] / 1024 << "MB total" << std::endl;
    std::cout << "Virtual memory: " << ramInfo["VmallocTotal"] / 1024 << "MB" << std::endl;
    std::cout << "Processors: " << get_nprocs() << std::endl;
    std::cout << "Load average: " << std::fixed << std::setprecision(2)
        << loadAverage.at(0) << ", " << loadAverage.at(1) << ", " << loadAverage.at(2) << std::endl;

    std::cout << "Drives:" << std::endl;
    const auto maxLengthPath = mounts.at(mounts.size() - 1).mountPoint.size();
    for (const auto& mount : mounts)
    {
        std::cout << "  " << mount.mountPoint;

        for (int i = 0; i < maxLengthPath - mount.mountPoint.size(); i++)
        {
            std::cout << " ";
        }

        std::cout << "  " << mount.fsType << "  "
            << mount.freeBytes / 1024 / 1024 / 1024 << "GB free / "
            << mount.totalBytes / 1024 / 1024 / 1024 << "GB total" << std::endl;
    }

    return 0;
}