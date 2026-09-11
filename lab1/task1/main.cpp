#include <cstring>
#include <windows.h>
#include <iostream>
#include <psapi.h>
#include <VersionHelpers.h>
#include <vector>

constexpr auto MAX_COMPUTER_NAME_LENGTH = 256;
constexpr auto MAX_FILESYSTEM_LENGTH = 30;
constexpr auto MB = 1024ull * 1024ull;
constexpr auto GB = MB * 1024ull;

std::string GetOSVersionInfo() {
    std::string osVersion = "unknown";

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

std::string GetComputerNameWrapper() {
    TCHAR  buff[MAX_COMPUTER_NAME_LENGTH] = {'\0'};
    DWORD  bufCharCount = MAX_COMPUTER_NAME_LENGTH;
    if(!GetComputerName(buff, &bufCharCount)) {
        throw std::runtime_error(std::to_string(GetLastError()));
    }
    return buff;
}

std::string GetUserNameWrapper() {
    TCHAR  buff[MAX_COMPUTER_NAME_LENGTH] = {'\0'};
    DWORD  bufCharCount = MAX_COMPUTER_NAME_LENGTH;
    if(!GetUserName(buff, &bufCharCount)) {
        throw std::runtime_error(std::to_string(GetLastError()));
    }
    return buff;
}

SYSTEM_INFO GetSysInfoWrapper() {
    SYSTEM_INFO siSysInfo;
    GetNativeSystemInfo(&siSysInfo);
    return siSysInfo;
}

std::string GetProcessorArchitecture(const SYSTEM_INFO& sysInfo) {
        switch (sysInfo.wProcessorArchitecture) {
            case PROCESSOR_ARCHITECTURE_INTEL:
                return "x86 (Intel)";
            case PROCESSOR_ARCHITECTURE_MIPS:
                return "MIPS";
            case PROCESSOR_ARCHITECTURE_ALPHA:
                return "Alpha";
            case PROCESSOR_ARCHITECTURE_PPC:
                return "PowerPC";
            case PROCESSOR_ARCHITECTURE_SHX:
                return "SHx";
            case PROCESSOR_ARCHITECTURE_ARM:
                return "ARM32";
            case PROCESSOR_ARCHITECTURE_IA64:
                return "IA64 (Itanium)";
            case PROCESSOR_ARCHITECTURE_ALPHA64:
                return "Alpha64";
            case PROCESSOR_ARCHITECTURE_MSIL:
                return "MSIL (.NET)";
            case PROCESSOR_ARCHITECTURE_AMD64:
                return "x64 (AMD64)";
            case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
            return "x86 (WOW64 on x64)";
            case PROCESSOR_ARCHITECTURE_NEUTRAL:
                return "Neutral (agnostic)";
            case PROCESSOR_ARCHITECTURE_ARM64:
            return "ARM64";
            case PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64:
                return "ARM32 (on Windows on ARM64)";
            case PROCESSOR_ARCHITECTURE_IA32_ON_ARM64:
                return "x86 (emulated on ARM64)";
            default:
                return "Unknown";
        }
    }

MEMORYSTATUSEX GetMemoryInfoWrapper() {
    MEMORYSTATUSEX memInfo{};
    memInfo.dwLength = sizeof(memInfo);
    if (!GlobalMemoryStatusEx(&memInfo)) {
        throw std::runtime_error(std::to_string(GetLastError()));
    }

    return memInfo;
}

PERFORMANCE_INFORMATION GetPerformanceInfoWrapper() {
    PERFORMANCE_INFORMATION perfInfo = {};
    perfInfo.cb = sizeof(PERFORMANCE_INFORMATION);

    if (!GetPerformanceInfo(&perfInfo, perfInfo.cb)) {
        throw std::runtime_error(std::to_string(GetLastError()));
    }

    return perfInfo;
}

struct Drive {
    std::string driveMountPoint;
    std::string driveType;
    unsigned long long totalBytes{};
    unsigned long long freeBytes{};
};

std::vector<std::string> GetLogicalDriveList() {
    std::vector<std::string> drives{};

    const DWORD size = GetLogicalDriveStrings(0, nullptr);
    if (size == 0) {
        throw std::runtime_error(std::to_string(GetLastError()));
    }

    std::vector<CHAR> buffer(size + 1, L'\0');
    const DWORD result = GetLogicalDriveStrings(size, buffer.data());
    if (result == 0) {
        throw std::runtime_error(std::to_string(GetLastError()));
    }

    const CHAR* symbolPtr = buffer.data();
    while (*symbolPtr) {
        drives.emplace_back(symbolPtr);
        symbolPtr += strlen(symbolPtr) + 1;
    }

    return drives;
}

std::string GetFileSystemType(const std::string& driveMountPoint) {
    CHAR fileSystemType[MAX_FILESYSTEM_LENGTH];
    if (GetVolumeInformation(
        driveMountPoint.c_str(),
        nullptr,
        0,
        nullptr,
        nullptr,
        nullptr,
        fileSystemType,
        MAX_FILESYSTEM_LENGTH + 1
    )) {
        return fileSystemType;
    }

    return "";
}

std::vector<Drive> GetLogicalDrivesInfo() {
    std::vector<Drive> drives{};

    const auto driveNamesList = GetLogicalDriveList();
    ULARGE_INTEGER freeToCaller, totalBytes, freeBytes;
    for (const auto& driveName : driveNamesList) {
        if (GetDiskFreeSpaceEx(
            driveName.data(),
            &freeToCaller,
            &totalBytes,
            &freeBytes
        )) {
            drives.emplace_back(
                driveName,
                GetFileSystemType(driveName),
                totalBytes.QuadPart,
                freeBytes.QuadPart
            );
        }
    }

    return drives;
}

int main() {
    const auto sysInfo = GetSysInfoWrapper();
    const auto memInfo = GetMemoryInfoWrapper();
    const auto performanceInfo = GetPerformanceInfoWrapper();
    const auto drives = GetLogicalDrivesInfo();

    std::cout << "OS: " << GetOSVersionInfo() << std::endl
    << "Computer Name: " << GetComputerNameWrapper() << std::endl
    << "User: " << GetUserNameWrapper() << std::endl
    << "Architecture: " << GetProcessorArchitecture(sysInfo) << std::endl
    << "RAM: " << memInfo.ullAvailPhys / MB << "MB free / " << memInfo.ullTotalPhys / MB << "MB total" << std::endl
    << "Swap: " << performanceInfo.CommitLimit * performanceInfo.PageSize / MB << "MB free / "
        << performanceInfo.CommitTotal * performanceInfo.PageSize / MB << "MB total" << std::endl
    << "Virtual Memory: " << memInfo.ullTotalVirtual / MB << "MB" << std::endl
    << "Memory Load: " << memInfo.dwMemoryLoad << "%" << std::endl
    << "Pagefile: " << memInfo.ullAvailPageFile / MB << "MB free / " << memInfo.ullTotalPageFile / MB << "MB total" << std::endl
    << std::endl
    << "Processors: " << sysInfo.dwNumberOfProcessors << std::endl
    << "Drives: " << std::endl;

    for (auto& drive : drives) {
        std::cout << " - " << drive.driveMountPoint << "  (" << drive.driveType << "): "
        << drive.freeBytes / GB  << "GB free / " << drive.totalBytes / GB << "GB total" << std::endl;
    }
}