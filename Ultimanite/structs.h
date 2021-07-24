#pragma once
#include "framework.h"

class StructMaker
{
public:
	void* structPtr = nullptr;
private:
	size_t totalSize = 0;
	size_t padding = 0;

	template <typename T>
	void add(T Src)
	{
		constexpr auto Size = sizeof(T);

		totalSize += Size;

		structPtr = realloc(structPtr, totalSize);
		if (structPtr) memcpy((void*)(reinterpret_cast<uintptr_t>(structPtr) + padding), &Src, Size);

		//printf("Size: %d, totalSize: %d, paddint: %d\n", Size, totalSize, padding);

		padding += Size;
	}

	template <class none = void>
	constexpr void* Create() const
	{
		return (void*)((uintptr_t)structPtr + padding);
	}

public:
	template <typename T, typename... Rest>
	void* Create(T retValue, Rest ... params)
	{
		add(retValue);

		return Create<>(params...);
	}
};

template <class T>
struct TArray
{
	friend struct FString;

public:
	T* Data;
	int32_t Count;
	int32_t Max;

	inline TArray()
	{
		Data = nullptr;
		Count = Max = 0;
	};

	inline int Num() const
	{
		return Count;
	};

	inline T& operator[](int i)
	{
		return Data[i];
	};

	inline const T& operator[](int i) const
	{
		return Data[i];
	};

	inline bool IsValidIndex(int i) const
	{
		return i < Num();
	}

	inline void Add(T InputData)
	{
		Data = (T*)realloc(Data, sizeof(T) * (Count + 1));
		Data[Count++] = InputData;
		Max = Count;
	};
};

struct FString : private TArray<wchar_t>
{
	FString()
	{
	};

	FString(const wchar_t* other)
	{
		Max = Count = *other ? std::wcslen(other) + 1 : 0;

		if (Count)
		{
			Data = const_cast<wchar_t*>(other);
		}
	}

	bool IsValid() const
	{
		return Data != nullptr;
	}

	const wchar_t* ToWString() const
	{
		return Data;
	}

	std::string ToString() const
	{
		auto length = std::wcslen(Data);

		std::string str(length, '\0');

		std::use_facet<std::ctype<wchar_t>>(std::locale()).narrow(Data, Data + length, '?', &str[0]);

		return str;
	}
};

struct FName;

void (*FNameToString)(FName* pThis, FString& out);

struct FName
{
	uint32_t ComparisonIndex;
	uint32_t DisplayIndex;

	FName() = default;

	explicit FName(int64_t name)
	{
		DisplayIndex = (name & 0xFFFFFFFF00000000LL) >> 32;
		ComparisonIndex = (name & 0xFFFFFFFFLL);
	};

	FName(uint32_t comparisonIndex, uint32_t displayIndex) : ComparisonIndex(comparisonIndex),
		DisplayIndex(displayIndex)
	{
	}

	auto ToString()
	{
		FString temp;
		FNameToString(this, temp);

		std::wstring ret(temp.ToWString());

		return ret;
	}
};

void* (*ProcessEvent)(void* Object, void* Function, void* Params);

struct UObject
{
	void** VTableObject;
	DWORD ObjectFlags;
	DWORD InternalIndex;
	UObject* Class;
	FName NamePrivate;
	UObject* Outer;

	bool IsA(UObject* cmp) const
	{
		if (this->Class == cmp)
		{
			return true;
		}
		return false;
	}

	std::wstring GetName()
	{
		return NamePrivate.ToString();
	}

	std::string GetNameA()
	{
		auto NameW = NamePrivate.ToString();
		return std::string(NameW.begin(), NameW.end());
	}

	std::wstring GetFullName()
	{
		std::wstring temp;

		for (auto outer = Outer; outer; outer = outer->Outer)
		{
			temp = outer->GetName() + L"." + temp;
		}

		temp = reinterpret_cast<UObject*>(Class)->GetName() + L" " + temp + this->GetName();
		return temp;
	}

	FName GetFName() const
	{
		return *reinterpret_cast<const FName*>(this + 0x18);
	}

	bool isValid() const
	{
		return !!Util::IsBadReadPtr((void*)this);
	}


	//TODO: fix return values!
	template <typename ReturnType = void*, typename First, typename ... Rest>
	inline ReturnType Call(UObject* function, First&& firstParam, Rest&&... params)
	{
		ReturnType RetInstance{};

		auto caller = new StructMaker();

		auto ret = *(ReturnType*)caller->Create(std::forward<First>(firstParam), std::forward<Rest>(params)...,
		                                        RetInstance);

		ProcessEvent(this, function, caller->structPtr);

		return ret;
	}
};

struct UField : UObject
{
    UField* Next;
};

struct UStruct : UField
{
    UStruct* Super; // 0x30
    struct UProperty* Children; // 0x38
    uint32_t Size; // 0x40
    char pad_44[0x88 - 0x30 - 0x14];
};
struct UFunction : UStruct
{
    uint32_t FunctionFlags;
    char pad[28]; // 0x8C
    void* Func; // 0xB0
};

struct FRotator
{
	float Pitch;
	float Yaw;
	float Roll;
};

struct FVector
{
	float X;
	float Y;
	float Z;
};

struct FVector2D
{
	float X;
	float Y;

	inline FVector2D()
		: X(0), Y(0)
	{
	}

	inline FVector2D(float x, float y)
		: X(x),
		Y(y)
	{
	}
};

struct FUObjectItem
{
	UObject* Object;
	DWORD Flags;
	DWORD ClusterIndex;
	DWORD SerialNumber;
	DWORD SerialNumber2;
};

struct TUObjectArray
{
	uint8_t* Objects;
	uint32_t MaxElements;
	uint32_t NumElements;
};

struct FActorSpawnParameters
{
	FActorSpawnParameters() : Name(), Template(nullptr), Owner(nullptr), Instigator(nullptr), OverrideLevel(nullptr),
		SpawnCollisionHandlingOverride(), bRemoteOwned(0), bNoFail(0),
		bDeferConstruction(0),
		bAllowDuringConstructionScript(0),
		NameMode(),
		ObjectFlags()
	{
	}
	;

	FName Name;
	UObject* Template;
	UObject* Owner;
	UObject* Instigator;
	UObject* OverrideLevel;
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride;

private:
	uint8_t bRemoteOwned : 1;

public:
	bool IsRemoteOwned() const { return bRemoteOwned; }

	uint8_t bNoFail : 1;
	uint8_t bDeferConstruction : 1;
	uint8_t bAllowDuringConstructionScript : 1;
	ESpawnActorNameMode NameMode;
	EObjectFlags ObjectFlags;
};