#pragma once
#include <algorithm>
#include <utility>
#include <vector>

#include "ContainerAllocator.h"


template <typename T, typename Allocator>
class TArray
{
public:
    using SizeType = typename Allocator::size_type;

private:
    std::vector<T, Allocator> ContainerPrivate;

public:
    // Iterator를 사용하기 위함
    auto begin() noexcept { return ContainerPrivate.begin(); }
    auto end() noexcept { return ContainerPrivate.end(); }
    auto begin() const noexcept { return ContainerPrivate.begin(); }
    auto end() const noexcept { return ContainerPrivate.end(); }
    auto rbegin() noexcept { return ContainerPrivate.rbegin(); }
    auto rend() noexcept { return ContainerPrivate.rend(); }
    auto rbegin() const noexcept { return ContainerPrivate.rbegin(); }
    auto rend() const noexcept { return ContainerPrivate.rend(); }

    T& operator[](SizeType Index);
    const T& operator[](SizeType Index) const;
	void operator+(const TArray& OtherArray);

public:
    TArray();
    ~TArray() = default;

    // 이니셜라이저 생성자
    TArray(std::initializer_list<T> InitList);

    // 복사 생성자
    TArray(const TArray& Other);

    // 이동 생성자
    TArray(TArray&& Other) noexcept;

    // 복사 할당 연산자
    TArray& operator=(const TArray& Other);

    // 이동 할당 연산자
    TArray& operator=(TArray&& Other) noexcept;

	/** Element를 Number개 만큼 초기화 합니다. */
    void Init(const T& Element, SizeType Number);
    void Append(const TArray<T, Allocator>& OtherArray);
    void Append(const T* Ptr, SizeType Count);
    SizeType Add(const T& Item);
    SizeType Add(T&& Item);
    SizeType AddUnique(const T& Item);

	template <typename... Args>
    SizeType Emplace(Args&&... Item);

    /** Array가 비어있는지 확인합니다. */
    bool IsEmpty() const;

	/** Array를 비웁니다 */
    void Empty(SizeType Slack = 0);

	/** Item과 일치하는 모든 요소를 제거합니다. */
    SizeType Remove(const T& Item);

	/** 왼쪽부터 Item과 일치하는 요소를 1개 제거합니다. */
    bool RemoveSingle(const T& Item);

	/** 특정 위치에 있는 요소를 제거합니다. */
    void RemoveAt(SizeType Index);

	/** Predicate에 부합하는 모든 요소를 제거합니다. */
    template <typename Predicate>
        requires std::is_invocable_r_v<bool, Predicate, const T&>
    SizeType RemoveAll(const Predicate& Pred);

    T* GetData();
    const T* GetData() const;



    /**
     * Array에서 Item을 찾습니다.
     * @param Item 찾으려는 Item
     * @return Item의 인덱스, 찾을 수 없다면 -1
     */
    SizeType Find(const T& Item);
    bool Find(const T& Item, SizeType& Index);

    /** 요소가 존재하는지 확인합니다. */
    bool Contains(const T& Item) const;

    /** Array Size를 가져옵니다. */
    SizeType Num() const;

    /** Array의 Capacity를 가져옵니다. */
    SizeType Len() const;

	/** Array의 Size를 Number로 설정합니다. */
	void SetNum(SizeType Number);

	/** Array의 Capacity를 Number로 설정합니다. */
    void Reserve(SizeType Number);

    void Sort();
    template <typename Compare>
        requires std::is_invocable_r_v<bool, Compare, const T&, const T&>
    void Sort(const Compare& CompFn);

    bool IsValidIndex(uint32 ElementIndex) const {
        if (ElementIndex < 0 || ElementIndex >= Len()) return false;

        return true;
    }
};

/**
 * @brief std::vector의 내용을 TArray로 복사합니다.
 *
 * TArray의 기존 내용은 지워집니다. std::vector의 요소들이 TArray로 복사됩니다.
 * TArray 클래스가 Empty(SizeType PreallocateSize)와 Append(const T* Ptr, SizeType Count)
 * 멤버 함수를 제공한다고 가정합니다 (Unreal Engine의 TArray와 유사).
 *
 * @tparam T 요소 타입
 * @tparam Allocator TArray의 내부 할당자 타입
 * @tparam StdAllocator std::vector의 할당자 타입 (보통 신경쓰지 않아도 됨)
 * @param SourceVector 복사할 원본 std::vector
 * @param DestinationArray 데이터를 복사받을 대상 TArray
 */
template <typename T, typename Allocator, typename StdAllocator>
void ConvertStdVectorToTArray(const std::vector<T, StdAllocator>& SourceVector, TArray<T, Allocator>& DestinationArray)
{
    // 1. 원본 std::vector의 크기를 가져옵니다.
    //    TArray의 SizeType을 사용하는 것이 타입 일관성에 좋습니다.
    const typename TArray<T, Allocator>::SizeType SourceSize = SourceVector.size();

    // 2. 대상 TArray의 내용을 비우고, 원본 벡터 크기만큼 메모리를 미리 확보합니다.
    //    Empty 함수에 크기를 전달하면 내부적으로 reserve와 유사한 효과를 냅니다.
    DestinationArray.Empty(SourceSize);

    // 3. 원본 벡터의 데이터가 실제로 존재할 때만 Append를 호출합니다.
    //    (std::vector는 내부 데이터가 연속적임을 보장하므로 .data() 사용 가능)
    if (SourceSize > 0)
    {
        // Append 함수를 사용하여 원본 벡터의 데이터를 TArray로 효율적으로 복사합니다.
        DestinationArray.Append(SourceVector.data(), SourceSize);
    }

    // DestinationArray는 이제 SourceVector와 동일한 요소들을 포함하게 됩니다.
}

// std::vector의 Allocator를 명시하지 않는 더 간단한 오버로드
template <typename T, typename Allocator>
void ConvertStdVectorToTArray(const std::vector<T>& SourceVector, TArray<T, Allocator>& DestinationArray)
{
    const typename TArray<T, Allocator>::SizeType SourceSize = SourceVector.size();
    DestinationArray.Empty(SourceSize);
    if (SourceSize > 0)
    {
        DestinationArray.Append(SourceVector.data(), SourceSize);
    }
}



template <typename T, typename Allocator>
T& TArray<T, Allocator>::operator[](SizeType Index)
{
    return ContainerPrivate[Index];
}

template <typename T, typename Allocator>
const T& TArray<T, Allocator>::operator[](SizeType Index) const
{
    return ContainerPrivate[Index];
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::operator+(const TArray& OtherArray)
{
	ContainerPrivate.insert(end(), OtherArray.begin(), OtherArray.end());
}

template <typename T, typename Allocator>
TArray<T, Allocator>::TArray()
    : ContainerPrivate()
{
}

template <typename T, typename Allocator>
TArray<T, Allocator>::TArray(std::initializer_list<T> InitList)
    : ContainerPrivate(InitList)
{
}

template <typename T, typename Allocator>
TArray<T, Allocator>::TArray(const TArray& Other)
    : ContainerPrivate(Other.ContainerPrivate)
{
}

template <typename T, typename Allocator>
TArray<T, Allocator>::TArray(TArray&& Other) noexcept
    : ContainerPrivate(std::move(Other.ContainerPrivate))
{
}

template <typename T, typename Allocator>
TArray<T, Allocator>& TArray<T, Allocator>::operator=(const TArray& Other)
{
    if (this != &Other)
    {
        ContainerPrivate = Other.ContainerPrivate;
    }
    return *this;
}

template <typename T, typename Allocator>
TArray<T, Allocator>& TArray<T, Allocator>::operator=(TArray&& Other) noexcept
{
    if (this != &Other)
    {
        ContainerPrivate = std::move(Other.ContainerPrivate);
    }
    return *this;
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::Init(const T& Element, SizeType Number)
{
    ContainerPrivate.assign(Number, Element);
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::Append(const TArray<T, Allocator>& OtherArray)
{
    ContainerPrivate.insert(ContainerPrivate.end(), OtherArray.ContainerPrivate.begin(), OtherArray.ContainerPrivate.end());
}

template<typename T, typename Allocator>
inline void TArray<T, Allocator>::Append(const T* Ptr, SizeType Count)
{
    if (Ptr && Count > 0)
    {
        // ContainerPrivate의 현재 크기 + 추가될 크기만큼 예약 (선택적이지만 권장)
        ContainerPrivate.reserve(ContainerPrivate.size() + Count);
        // insert를 사용하여 데이터 범위 추가
        ContainerPrivate.insert(ContainerPrivate.end(), Ptr, Ptr + Count);

        // 또는 std::back_inserter 사용 (C++11 이상)
        // std::copy(Ptr, Ptr + Count, std::back_inserter(ContainerPrivate));
    }
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Add(const T& Item)
{
    return Emplace(Item);
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Add(T&& Item)
{
    return Emplace(std::move(Item));
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::AddUnique(const T& Item)
{
    if (SizeType Index; Find(Item, Index))
    {
        return Index;
    }
    return Add(Item);
}

template <typename T, typename Allocator>
template <typename... Args>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Emplace(Args&&... Item)
{
    ContainerPrivate.emplace_back(std::forward<Args>(Item)...);
    return Num()-1;
}

template <typename T, typename Allocator>
bool TArray<T, Allocator>::IsEmpty() const
{
    return ContainerPrivate.empty();
}


/**
 * 배열의 모든 요소를 제거합니다.
 * 선택적으로 Slack 파라미터를 지정하여, 제거 후에도 Slack 개수만큼의 요소를 저장할 수 있는
 * 메모리를 미리 확보할 수 있습니다.
 * @param Slack - 미리 확보할 요소의 수 (기본값 0)
 */
template<typename T, typename Allocator>
inline void TArray<T, Allocator>::Empty(SizeType Slack)
{
    // 1. 내부 std::vector의 내용을 모두 지웁니다.
    ContainerPrivate.clear();

    // 2. Slack 값이 0보다 크면 해당 크기만큼 메모리를 예약합니다.
    if (Slack > 0)
    {
        ContainerPrivate.reserve(Slack);
    }
    // Slack이 0일 경우, reserve를 호출하지 않습니다.
    // std::vector::clear()는 capacity를 변경하지 않으므로,
    // 메모리를 명시적으로 축소하려면 shrink_to_fit() 호출이 필요하지만,
    // Empty()의 일반적인 목적은 비우는 것이므로 보통 shrink_to_fit은 하지 않습니다.
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Remove(const T& Item)
{
    auto oldSize = ContainerPrivate.size();
    ContainerPrivate.erase(std::remove(ContainerPrivate.begin(), ContainerPrivate.end(), Item), ContainerPrivate.end());
    return static_cast<SizeType>(oldSize - ContainerPrivate.size());
}

template <typename T, typename Allocator>
bool TArray<T, Allocator>::RemoveSingle(const T& Item)
{
    auto it = std::find(ContainerPrivate.begin(), ContainerPrivate.end(), Item);
    if (it != ContainerPrivate.end())
    {
        ContainerPrivate.erase(it);
        return true;
    }
    return false;
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::RemoveAt(SizeType Index)
{
    if (Index >= 0 && static_cast<SizeType>(Index) < ContainerPrivate.size())
    {
        ContainerPrivate.erase(ContainerPrivate.begin() + Index);
    }
}

template <typename T, typename Allocator>
template <typename Predicate>
    requires std::is_invocable_r_v<bool, Predicate, const T&>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::RemoveAll(const Predicate& Pred)
{
    auto oldSize = ContainerPrivate.size();
    ContainerPrivate.erase(std::remove_if(ContainerPrivate.begin(), ContainerPrivate.end(), Pred), ContainerPrivate.end());
    return static_cast<SizeType>(oldSize - ContainerPrivate.size());
}

template <typename T, typename Allocator>
T* TArray<T, Allocator>::GetData()
{
    return ContainerPrivate.data();
}

template <typename T, typename Allocator>
const T* TArray<T, Allocator>::GetData() const
{
    return ContainerPrivate.data();
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Find(const T& Item)
{
    const auto it = std::find(ContainerPrivate.begin(), ContainerPrivate.end(), Item);
    return it != ContainerPrivate.end() ? std::distance(ContainerPrivate.begin(), it) : -1;
}

template <typename T, typename Allocator>
bool TArray<T, Allocator>::Find(const T& Item, SizeType& Index)
{
    Index = Find(Item);
    return (Index != -1);
}

template <typename T, typename Allocator>
bool TArray<T, Allocator>::Contains(const T& Item) const
{
    for (const T* Data = GetData(), *DataEnd = Data + Num(); Data != DataEnd; ++Data)
    {
        if (*Data == Item)
        {
            return true;
        }
    }
    return false;
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Num() const
{
    return ContainerPrivate.size();
}

template <typename T, typename Allocator>
typename TArray<T, Allocator>::SizeType TArray<T, Allocator>::Len() const
{
    return ContainerPrivate.capacity();
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::SetNum(SizeType Number)
{
	ContainerPrivate.resize(Number);
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::Reserve(SizeType Number)
{
    ContainerPrivate.reserve(Number);
}

template <typename T, typename Allocator>
void TArray<T, Allocator>::Sort()
{
    std::sort(ContainerPrivate.begin(), ContainerPrivate.end());
}

template <typename T, typename Allocator>
template <typename Compare>
    requires std::is_invocable_r_v<bool, Compare, const T&, const T&>
void TArray<T, Allocator>::Sort(const Compare& CompFn)
{
    std::sort(ContainerPrivate.begin(), ContainerPrivate.end(), CompFn);
}

template <typename T, typename Allocator = FDefaultAllocator<T>> class TArray;
