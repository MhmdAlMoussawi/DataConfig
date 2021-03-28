#include "DataConfig/Reader/DcPutbackReader.h"
#include "DataConfig/Misc/DcTemplateUtils.h"
#include "DataConfig/Misc/DcTypeUtils.h"
#include "DataConfig/Diagnostic/DcDiagnosticUtils.h"
#include "DataConfig/Diagnostic/DcDiagnosticReadWrite.h"

template<typename TData>
FDcResult PopAndCheckCachedValue(FDcPutbackReader* Self, FDcDataVariant& OutValue)
{
	check(Self->Cached.Num() > 0);

	OutValue = Self->Cached.Pop();
	EDcDataEntry Expected = DcTypeUtils::TDcDataEntryType<TData>::Value;
	if (OutValue.DataType != Expected)
		return DC_FAIL(DcDReadWrite, DataTypeMismatch)
			<< (int)Expected << (int)OutValue.DataType;

	return DcOk();
}

template<typename TData>
FORCEINLINE FDcResult TryUseCachedValue(FDcPutbackReader* Self, TData* OutPtr)
{
	FDcDataVariant Value;
	DC_TRY(PopAndCheckCachedValue<TData>(Self, Value));

	if (OutPtr)
		*OutPtr = Value.GetValue<TData>();

	return DcOk();
}

template<typename TData>
FORCEINLINE FDcResult TryUseCachedValue(FDcPutbackReader* Self)
{
	FDcDataVariant Value;
	DC_TRY(PopAndCheckCachedValue<TData>(Self, Value));
	return DcOk();
}

template<typename TData, typename TMethod, typename... TArgs>
FORCEINLINE FDcResult CachedRead(FDcPutbackReader* Self, TMethod Method, TArgs&&... Args)
{
	if (Self->Cached.Num() > 0)
	{
		return TryUseCachedValue<TData>(Self, Forward<TArgs>(Args)...);
	}
	else
	{
		return (Self->Reader->*Method)(Forward<TArgs>(Args)...);
	}
}

template<typename TMethod, typename... TArgs>
FORCEINLINE FDcResult CanNotCachedRead(FDcPutbackReader* Self, EDcDataEntry Entry, TMethod Method, TArgs&&... Args)
{
	if (Self->Cached.Num() > 0)
		return DC_FAIL(DcDReadWrite, CantUsePutbackValue) << Entry;

	return (Self->Reader->*Method)(Forward<TArgs>(Args)...);
}

template<typename TMethod>
FORCEINLINE FDcResult CachedReadDataTypeOnly(FDcPutbackReader* Self, EDcDataEntry Entry, TMethod Method)
{
	if (Self->Cached.Num() > 0)
	{
		FDcDataVariant Value = Self->Cached.Pop();
		check(Value.bDataTypeOnly);
		if (Value.DataType != Entry)
			return DC_FAIL(DcDReadWrite, DataTypeMismatch)
				<< Entry << Value.DataType;

		return DcOk();
	}
	else
	{
		return (Self->Reader->*Method)();
	}
}


FDcResult FDcPutbackReader::PeekRead(EDcDataEntry* OutPtr)
{
	if (Cached.Num() > 0)
	{
		*OutPtr = Cached.Last().DataType;
		return DcOk();
	}
	else
	{
		return Reader->PeekRead(OutPtr);
	}
}

FDcResult FDcPutbackReader::ReadNil()
{
	return CachedRead<nullptr_t>(this, &FDcReader::ReadNil);
}

FDcResult FDcPutbackReader::ReadBool(bool* OutPtr)
{
	return CachedRead<bool>(this, &FDcReader::ReadBool, OutPtr);
}

FDcResult FDcPutbackReader::ReadName(FName* OutPtr)
{
	return CachedRead<FName>(this, &FDcReader::ReadName, OutPtr);
}

FDcResult FDcPutbackReader::ReadString(FString* OutPtr)
{
	return CachedRead<FString>(this, &FDcReader::ReadString, OutPtr);
}

FDcResult FDcPutbackReader::ReadText(FText* OutPtr)
{
	return CachedRead<FText>(this, &FDcReader::ReadText, OutPtr);
}

FDcResult FDcPutbackReader::ReadEnum(FDcEnumData* OutPtr)
{
	return CachedRead<FDcEnumData>(this, &FDcReader::ReadEnum, OutPtr);
}

FDcResult FDcPutbackReader::ReadStructRoot(FDcStructStat* OutStructPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::StructRoot, &FDcReader::ReadStructRoot, OutStructPtr);
}

FDcResult FDcPutbackReader::ReadStructEnd(FDcStructStat* OutStructPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::StructEnd, &FDcReader::ReadStructEnd, OutStructPtr);
}

FDcResult FDcPutbackReader::ReadClassRoot(FDcClassStat* OutClassPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::ClassRoot, &FDcReader::ReadClassRoot, OutClassPtr);
}

FDcResult FDcPutbackReader::ReadClassEnd(FDcClassStat* OutClassPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::ClassEnd, &FDcReader::ReadClassEnd, OutClassPtr);
}

FDcResult FDcPutbackReader::ReadMapRoot()
{
	return CachedReadDataTypeOnly(this, EDcDataEntry::MapRoot, &FDcReader::ReadMapRoot);
}

FDcResult FDcPutbackReader::ReadMapEnd()
{
	return CachedReadDataTypeOnly(this, EDcDataEntry::MapEnd, &FDcReader::ReadMapEnd);
}

FDcResult FDcPutbackReader::ReadArrayRoot()
{
	return CachedReadDataTypeOnly(this, EDcDataEntry::ArrayRoot, &FDcReader::ReadArrayRoot);
}

FDcResult FDcPutbackReader::ReadArrayEnd()
{
	return CachedReadDataTypeOnly(this, EDcDataEntry::ArrayEnd, &FDcReader::ReadArrayEnd);
}

FDcResult FDcPutbackReader::ReadObjectReference(UObject** OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::ObjectReference, &FDcReader::ReadObjectReference, OutPtr);
}

FDcResult FDcPutbackReader::ReadClassReference(UClass** OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::ClassReference, &FDcReader::ReadClassReference, OutPtr);
}

FDcResult FDcPutbackReader::ReadWeakObjectReference(FWeakObjectPtr* OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::WeakObjectReference, &FDcReader::ReadWeakObjectReference, OutPtr);
}

FDcResult FDcPutbackReader::ReadLazyObjectReference(FLazyObjectPtr* OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::LazyObjectReference, &FDcReader::ReadLazyObjectReference, OutPtr);
}

FDcResult FDcPutbackReader::ReadSoftObjectReference(FSoftObjectPath* OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::SoftObjectReference, &FDcReader::ReadSoftObjectReference, OutPtr);
}

FDcResult FDcPutbackReader::ReadSoftClassReference(FSoftClassPath* OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::SoftClassReference, &FDcReader::ReadSoftClassReference, OutPtr);
}

FDcResult FDcPutbackReader::ReadInterfaceReference(FScriptInterface* OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::InterfaceReference, &FDcReader::ReadInterfaceReference, OutPtr);
}

FDcResult FDcPutbackReader::ReadFieldPath(FFieldPath* OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::FieldPath, &FDcReader::ReadFieldPath, OutPtr);
}

FDcResult FDcPutbackReader::ReadDelegate(FScriptDelegate* OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::Delegate, &FDcReader::ReadDelegate, OutPtr);
}

FDcResult FDcPutbackReader::ReadMulticastInlineDelegate(FMulticastScriptDelegate* OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::MulticastInlineDelegate, &FDcReader::ReadMulticastInlineDelegate, OutPtr);
}

FDcResult FDcPutbackReader::ReadMulticastSparseDelegate(FMulticastScriptDelegate* OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::MulticastSparseDelegate, &FDcReader::ReadMulticastSparseDelegate, OutPtr);
}

FDcResult FDcPutbackReader::ReadSetRoot()
{
	return CanNotCachedRead(this, EDcDataEntry::SetRoot, &FDcReader::ReadSetRoot);
}

FDcResult FDcPutbackReader::ReadSetEnd()
{
	return CanNotCachedRead(this, EDcDataEntry::SetEnd, &FDcReader::ReadSetEnd);
}

FDcResult FDcPutbackReader::ReadInt8(int8* OutPtr)
{
	return CachedRead<int8>(this, &FDcReader::ReadInt8, OutPtr);
}

FDcResult FDcPutbackReader::ReadInt16(int16* OutPtr)
{
	return CachedRead<int16>(this, &FDcReader::ReadInt16, OutPtr);
}

FDcResult FDcPutbackReader::ReadInt32(int32* OutPtr)
{
	return CachedRead<int32>(this, &FDcReader::ReadInt32, OutPtr);
}

FDcResult FDcPutbackReader::ReadInt64(int64* OutPtr)
{
	return CachedRead<int64>(this, &FDcReader::ReadInt64, OutPtr);
}

FDcResult FDcPutbackReader::ReadUInt8(uint8* OutPtr)
{
	return CachedRead<uint8>(this, &FDcReader::ReadUInt8, OutPtr);
}

FDcResult FDcPutbackReader::ReadUInt16(uint16* OutPtr)
{
	return CachedRead<uint16>(this, &FDcReader::ReadUInt16, OutPtr);
}

FDcResult FDcPutbackReader::ReadUInt32(uint32* OutPtr)
{
	return CachedRead<uint32>(this, &FDcReader::ReadUInt32, OutPtr);
}

FDcResult FDcPutbackReader::ReadUInt64(uint64* OutPtr)
{
	return CachedRead<uint64>(this, &FDcReader::ReadUInt64, OutPtr);
}

FDcResult FDcPutbackReader::ReadFloat(float* OutPtr)
{
	return CachedRead<float>(this, &FDcReader::ReadFloat, OutPtr);
}

FDcResult FDcPutbackReader::ReadDouble(double* OutPtr)
{
	return CachedRead<double>(this, &FDcReader::ReadDouble, OutPtr);
}

FDcResult FDcPutbackReader::ReadBlob(FDcBlobViewData* OutPtr)
{
	return CanNotCachedRead(this, EDcDataEntry::Blob, &FDcReader::ReadBlob, OutPtr);
}

bool FDcPutbackReader::Coercion(EDcDataEntry ToEntry)
{
	if (Cached.Num())
		return false;

	return Reader->Coercion(ToEntry);
}

void FDcPutbackReader::FormatDiagnostic(FDcDiagnostic& Diag)
{
	Reader->FormatDiagnostic(Diag);

	if (Cached.Num())
	{
		FDcDiagnosticHighlight Highlight(this, "PutbackReader");
		Highlight.Formatted = FString::Printf(TEXT("(Putback: %d)"), Cached.Num());
		Diag << MoveTemp(Highlight);
	}
}