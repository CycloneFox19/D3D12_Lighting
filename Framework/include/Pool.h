#pragma once

#include <cstdint>
#include <mutex>
#include <cassert>
#include <functional>

template<typename T>
class Pool
{


public:

	Pool()
		: m_pBuffer(nullptr)
		, m_pActive(nullptr)
		, m_pFree(nullptr)
		, m_Capacity(0)
		, m_Count(0)
	{
		// Do Nothing//
	}

	~Pool()
	{
		Term();
	}

	//! @brief initialize
	//! 
	//! @param[in] count 確保するアイテム数
	//! @retval true : successfully initialized
	//! @retval false : failed to initialize
	bool Init(uint32_t count)
	{
		std::lock_guard<std::mutex> guard(m_Mutex);

		m_pBuffer = static_cast<uint8_t*>(malloc(sizeof(Item) * (count + 2)));
		if (m_pBuffer == nullptr)
		{
			return false;
		}

		m_Capacity = count;

		for (auto i = 2u, j = 0u; i < m_Capacity + 2; ++i, ++j)
		{
			auto item = GetItem(i);
			item->m_Index = j;
		}

		m_pActive = GetItem(0);
		m_pActive->m_pPrev = m_pActive->m_pNext = m_pActive;
		m_pActive->m_Index = uint32_t(-1);

		m_pFree = GetItem(1);
		m_pFree->m_Index = uint32_t(-2);

		for (auto i = 1u; i < m_Capacity + 2; ++i)
		{
			GetItem(i)->m_pPrev = nullptr;
			GetItem(i)->m_pNext = GetItem(i + 1);
		}

		GetItem(m_Capacity + 1)->m_pPrev = m_pFree;

		m_Count = 0;

		return true;
	}

	//! @brief end
	void Term()
	{
		std::lock_guard<std::mutex> guard(m_Mutex);

		if (m_pBuffer) // has the same meaning as "if (m_pBuffer != nullptr)
		{
			free(m_pBuffer);
			m_pBuffer = nullptr;
		}
		m_pActive = nullptr;
		m_pFree = nullptr;
		m_Capacity = 0;
		m_Count = 0;
	}

	//! @brief アイテムを確保
	//! 
	//! @param[in] func ユーザによる初期化処理
	//! @return 確保したアイテムへのポインタ, 確保に失敗したときnullptr
	T* Alloc(std::function<void(uint32_t, T*)> func = nullptr)
	{
		std::lock_guard<std::mutex> guard(m_Mutex);

		if (m_pFree->m_pNext == m_pFree || m_Count + 1 > m_Capacity)
		{
			return nullptr;
		}

		auto item = m_pFree->m_pNext;
		m_pFree->m_pNext = item->m_pNext;

		item->m_pPrev = m_pActive->m_pPrev;
		item->m_pNext = m_pActive;
		item->m_pPrev->m_pNext = item->m_pNext->m_pPrev = item;

		m_Count++;

		//メモリ割り当て
		auto val = new((void*)item) T();

		if (func != nullptr)
		{
			func(item->m_Index, val);
		}

		return val;
	}

	//! @brief free the item
	//! 
	//! @param[in] pValue pointer to the item to be free
	void Free(T* pValue)
	{
		if (pValue == nullptr)
		{
			return;
		}

		std::lock_guard<std::mutex> guard(m_Mutex);

		auto item = reinterpret_cast<Item*>(pValue);

		item->m_pPrev->m_pNext = item->m_pNext;
		item->m_pNext->m_pPrev = item->m_pPrev;

		item->m_pPrev = nullptr;
		item->m_pNext = m_pFree->m_pNext;

		m_pFree->m_pNext = item;
		m_Count--;
	}

	//! @brief get total item amount
	//! 
	//! @return return total item amount
	uint32_t GetSize() const
	{
		return m_Capacity;
	}

	//! @brief get currently used item amount
	//! 
	//! @return return currently used item amount
	uint32_t GetUsedCount() const
	{
		return m_Count;
	}

	//! @brief get available item amount
	//! 
	//! @return return available item amount
	uint32_t GetAvailableCount() const
	{
		return m_Capacity - m_Count;
	}

private:

	//Item structure
	struct Item
	{
		T m_Value;
		uint32_t m_Index;
		Item* m_pNext;
		Item* m_pPrev;

		Item()
			: m_Value()
			, m_Index(0)
			, m_pNext(nullptr)
			, m_pPrev(nullptr)
		{
			// Do Nothing//
		}

		~Item()
		{
			// Do Nothing//
		}
	};

	//private variables
	uint8_t* m_pBuffer;
	Item* m_pActive;
	Item* m_pFree;
	uint32_t m_Capacity;
	uint32_t m_Count;
	std::mutex m_Mutex;

	//private methods
	
	//! @brief get item
	//! 
	//! @param[in] index index of item to get
	//! @return return pointer to the item
	Item* GetItem(uint32_t index)
	{
		assert(0 <= index && index <= m_Capacity + 2);
		return reinterpret_cast<Item*>(m_pBuffer + sizeof(Item) * index);
	}

	//! @brief アイテムにメモリを割り当て
	//! 
	//! @param[in] index index of the item to get
	//! @return return pointer to the item
	Item* AssignItem(uint32_t index)
	{
		assert(0 <= index && index <= m_Capacity + 2);
		auto buf = (m_pBuffer + sizeof(Item) * index);
		return new (buf) Item;
	}

	Pool(const Pool&) = delete;
	void operator = (const Pool&) = delete;
};
