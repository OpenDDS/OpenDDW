#pragma once

#include <filesystem>
#include <string>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace pi
{
    /// Returns the full path to the executable currently running.
    inline std::filesystem::path GetExecutablePath()
    {
        const int nPathIncrementSize = 200;
        std::string strExecutablePath;

#if defined( WIN32 ) || defined( WIN64 )
        DWORD nBufferExePathSize = 0;
        DWORD nSizeReturned = 0;
        // GetModuleFileName returns the size copied to the supplied buffer, if it
        // fills the entire buffer then the file path was too long and it truncated
        // it. Resize buffer and try again until the full file path is successfully
        // retrieved.

#if defined(UNICODE)
        std::vector<WCHAR> pathVector;  //WCHAR vector gets converted to a vector of chars
        while (nSizeReturned == nBufferExePathSize) {
            nBufferExePathSize += nPathIncrementSize;
            pathVector.resize(nBufferExePathSize, '\0');
            nSizeReturned = GetModuleFileName(nullptr, pathVector.data(), nBufferExePathSize);
        }
        // Resize one last time now that the final size has been determined
        pathVector.resize(nSizeReturned);
        strExecutablePath.resize(nSizeReturned);

        wcstombs(&strExecutablePath[0], pathVector.data(), pathVector.size());
#else
        while (nSizeReturned == nBufferExePathSize) {
            nBufferExePathSize += nPathIncrementSize;
            strExecutablePath.resize(nBufferExePathSize, '\0');
            nSizeReturned = GetModuleFileName(nullptr, &strExecutablePath[0], nBufferExePathSize);
        }
        // Resize one last time now that the final size has been determined
        strExecutablePath.resize(nSizeReturned);
#endif

#elif defined(__APPLE__)
        uint32_t nBufferExePathSize = nPathIncrementSize;
        strExecutablePath.resize(nBufferExePathSize, '\0');
        int ret = _NSGetExecutablePath(&strExecutablePath[0], &nBufferExePathSize);
        if (ret != 0) {
          strExecutablePath.resize(nBufferExePathSize, '\0');
          ret = _NSGetExecutablePath(&strExecutablePath[0], &nBufferExePathSize);
          if (ret != 0) {
            throw std::runtime_error("Error getting the executable path");
          }
        }

#else  // !defined( WIN32 ) && !defined( WIN64 ) && !defined(__APPLE__)
        size_t  nBufferExePathSize = 0;
        ssize_t nSizeReturned = 0;
        // readlink returns the size copied to the supplied buffer, if it fills the
        // entire buffer then the file path was too long and it truncated it. Resize
        // buffer and try again until the full file path is successfully retrieved.
        while (nBufferExePathSize == static_cast<size_t>(nSizeReturned)) {
            nBufferExePathSize += nPathIncrementSize;
            strExecutablePath.resize(nBufferExePathSize, '\0');
            nSizeReturned = readlink("/proc/self/exe", &strExecutablePath[0], nBufferExePathSize);
            if (nSizeReturned < 0) {
                throw std::runtime_error("Error finding the executable path.");
            }
        }
        strExecutablePath.resize(nSizeReturned);
#endif

        return std::filesystem::path(strExecutablePath);
    }

    /// Returns the full path of the directory containing the executable currently running.
    inline std::filesystem::path GetExecutableDirectory()
    {
        std::filesystem::path result = GetExecutablePath();
        result.remove_filename();
        return result;
    }


    inline std::string GetEnvVar(const std::string& var)
    {
        std::string res;

#ifdef WIN32
        size_t varLength;
        getenv_s(&varLength, nullptr, 0, var.c_str());
        if (varLength > 0) {
            res.resize(varLength);
            getenv_s(&varLength,
                &res[0],
                res.size(),
                var.c_str());
            res.resize(varLength - 1);    // remove null
        }
#else
        const char* s = getenv(var.c_str());
        if (s != nullptr) {
            res = s;
        }
#endif

        return res;
    }
}
