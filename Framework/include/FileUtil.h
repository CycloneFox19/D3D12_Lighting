#pragma once

#include <string>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

//! @brief search for file path
//! 
//! @param[in] filePath searching file path
//! @param[out] result container of the result
//! @retval true file detected
//! @retval false file not found
//! @memo the follwing is rule of searching
//!		.\
//!		..\
//!		..\..\
//!		.\res\
//!		%EXE_DIR%\
//!		%EXE_DIR%\..\
//!		%EXE_DIR%\..\..\
//!		%EXE_DIR%\res\
//! 
bool SearchFilePathA(const char* filename, std::string& result);

//! @brief search for file path
//! 
//! @param[in] filePath searching file path
//! @param[out] result container of the result
//! @retval true file detected
//! @retval false file not found
//! @memo the follwing is rule of searching
//!		.\
//!		..\
//!		..\..\
//!		.\res\
//!		%EXE_DIR%\
//!		%EXE_DIR%\..\
//!		%EXE_DIR%\..\..\
//!		%EXE_DIR%\res\
//! 
bool SearchFilePathW(const wchar_t* filename, std::wstring& result);

//! @brief remove directory path and return filename
//! 
//! @param[in] path filepath to remove filepath
//! @return return filename
std::string RemoveDirectoryPathA(const std::string& path);

//! @brief remove directory path and return filename
//! 
//! @param[in] path filepath to remove filepath
//! @return return filename
std::wstring RemoveDirectoryPathW(const std::wstring& path);

//! @brief get directory name
//! 
//! @param[in] path file path
//! @return return file name
std::string GetDirectoryPathA(const char* path);

//! @brief get directory name
//! 
//! @param[in] path file path
//! @return return file name
std::wstring GetDirectoryPathW(const wchar_t* path);

#if defined(UNICODE) || defined(_UNICODE)
inline bool SearchFilePath(const wchar_t* filename, std::wstring& result)
{
	return SearchFilePathW(filename, result);
}

inline std::wstring RemoveDirectoryPath(const std::wstring& path)
{
	return RemoveDirectoryPathW(path);
}

inline std::wstring GetDirectoryPath(const wchar_t* path)
{
	return GetDirectoryPathW(path);
}
#else
inline bool SearchFilePath(const char* filename, std::string& result)
{
	return SearchFilePathA(filename, result);
}

inline std::string RemoveDirectoryPath(const std::string& path)
{
	return RemoveDirectoryPathA(path);
}

inline std::string GetDirectoryPath(const char* path)
{
	return GetDirectoryPathA(path);
}
#endif //defined(UNICODE) || defined(_UNICODE)
