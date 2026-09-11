#include <string>
#include <cstdint>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <cstring>
#include <windows.h>
#include <iostream>
#include <VersionHelpers.h>

constexpr auto MAX_COMPUTER_NAME_LENGTH = 256;
constexpr auto MAX_FILESYSTEM_LENGTH = 30;
constexpr auto MB = 1024ull * 1024ull;
constexpr auto GB = MB * 1024ull;

#else
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
#endif

class SysInfo
{
public:
    std::string GetOSName() const;
    std::string GetOSVersion() const;
    uint64_t GetFreeMemory() const;
    uint64_t GetTotalMemory() const;
    unsigned GetProcessorCount() const;
};

#ifdef _WIN32

std::string GetOSVersionInfo() {
    std::string osVersion = "Unknown";

    if (IsWindows10OrGreater()) {
        osVersion = "Windows 10 Or Greater";
    }
    else if (IsWindows8Point1OrGreater()) {
        osVersion = "Windows 8.1";
    }
    else if (IsWindows8OrGreater()) {
        osVersion = "Windows 8";
    }
    else if (IsWindows7SP1OrGreater()) {
        osVersion = "Windows 7 SP 1";
    }
    else if (IsWindows7OrGreater()) {
        osVersion = "Windows 7";
    }
    else if (IsWindowsVistaSP2OrGreater()) {
        osVersion = "VistaSP 2";
    }
    else if (IsWindowsVistaSP1OrGreater()) {
        osVersion = "VistaSP 1";
    }
    else if (IsWindowsVistaOrGreater()) {
        osVersion = "Vista";
    }
    else if (IsWindowsXPSP3OrGreater()) {
        osVersion = "XPSP 3";
    }
    else if (IsWindowsXPSP2OrGreater()) {
        osVersion = "XPSP 2";
    }
    else if (IsWindowsXPSP1OrGreater()) {
        osVersion = "XPSP 1";
    }
    else if (IsWindowsXPOrGreater()) {
        osVersion = "XP";
    }

    return osVersion;
}

SYSTEM_INFO GetSysInfoWrapper() {
    SYSTEM_INFO siSysInfo;
    GetNativeSystemInfo(&siSysInfo);
    return siSysInfo;
}

MEMORYSTATUSEX GetMemoryInfoWrapper() {
    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(memInfo);
    if (!GlobalMemoryStatusEx(&memInfo)) {
        throw std::runtime_error(std::to_string(GetLastError()));
    }

    return memInfo;
}

std::string SysInfo::GetOSName() const {
    return "Windows";
}

std::string SysInfo::GetOSVersion() const {
    return GetOSVersionInfo();
}

uint64_t SysInfo::GetFreeMemory() const {
    const auto memInfo = GetMemoryInfoWrapper();
    return memInfo.ullAvailPhys;
}

uint64_t SysInfo::GetTotalMemory() const {
    const auto memInfo = GetMemoryInfoWrapper();
    return memInfo.ullTotalPhys;
}

unsigned SysInfo::GetProcessorCount() const {
    const auto sysInfo = GetSysInfoWrapper();
    return sysInfo.dwNumberOfProcessors;
}

#else

std::string Exec(const std::string& cmd) {
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

std::string GetOSVersion() {
    const auto rawVersionData = Exec("lsb_release --description");
    auto keyValuePair = Separate(rawVersionData, ':');
    if (keyValuePair.size() != 2)
    {
        throw std::runtime_error("[GetOSVersion] Error while parsing version info.");
    }
    return keyValuePair[1];
}

std::map<std::string, unsigned long> GetRandomAccessMemoryInfo() {
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

std::string SysInfo::GetOSName() const {
    return "Linux";
}

std::string SysInfo::GetOSVersion() const {
    return GetOSVersion();
}

uint64_t SysInfo::GetFreeMemory() const {
    auto ramInfo = GetRandomAccessMemoryInfo();
    return ramInfo["MemFree"] * 1024;
}

uint64_t SysInfo::GetTotalMemory() const {
    auto ramInfo = GetRandomAccessMemoryInfo();
    return ramInfo["MemTotal"] * 1024;
}

unsigned SysInfo::GetProcessorCount() const {
    return get_nprocs();
}

#endif

int main() {
    SysInfo sysInfo;
    std::cout << "OS: "<< sysInfo.GetOSName() << std::endl;
    std::cout << "OS version: " << sysInfo.GetOSVersion() << std::endl;
    std::cout << "Processors: " << sysInfo.GetProcessorCount() << std::endl;
    std::cout << "RAM: "<< sysInfo.GetFreeMemory() / GB << "GB free / "
        << sysInfo.GetTotalMemory() / GB << "GB total" << std::endl;
}