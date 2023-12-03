#pragma once

// delete allowing for nullptr
template<typename T>
inline void SafeDelete(T*& ptr)
{
	if (ptr != nullptr)
	{
		delete ptr;
		ptr = nullptr;
	}
}

// delete[] allowing for nullptr
template<typename T>
inline void SafeDeleteArray(T*& ptr)
{
	if (ptr != nullptr)
	{
		delete[] ptr;
		ptr = nullptr;
	}
}

// call release() method allowing for nullptr
template<typename T>
inline void SafeRelease(T*& ptr)
{
	if (ptr != nullptr)
	{
		ptr->Release();
		ptr = nullptr;
	}
}

// delete by calling Term() method allowing for nullptr
template<typename T>
inline void SafeTerm(T*& ptr)
{
	if (ptr != nullptr)
	{
		ptr->Term();
		delete ptr;
		ptr = nullptr;
	}
}
