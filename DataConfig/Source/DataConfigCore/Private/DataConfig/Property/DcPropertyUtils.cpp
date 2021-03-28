#include "DataConfig/Property/DcPropertyUtils.h"
#include "DataConfig/Property/DcPropertyDatum.h"
#include "DataConfig/DcEnv.h"
#include "DataConfig/Diagnostic/DcDiagnosticReadWrite.h"
#include "UObject/UnrealType.h"
#include "UObject/TextProperty.h"
#include "UObject/PropertyAccessUtil.h"

namespace DcPropertyUtils
{

bool IsEffectiveProperty(FProperty* Property)
{
	check(Property);
#if DC_BUILD_DEBUG
	//	this is for handling cases that UE4 added new property and we're not supporting it yet
	return Property->IsA<FBoolProperty>()
		|| Property->IsA<FNumericProperty>()
		|| Property->IsA<FStrProperty>()
		|| Property->IsA<FNameProperty>()
		|| Property->IsA<FTextProperty>()
		|| Property->IsA<FEnumProperty>()
		|| Property->IsA<FStructProperty>()
		|| Property->IsA<FClassProperty>()
		|| Property->IsA<FObjectProperty>()
		|| Property->IsA<FMapProperty>()
		|| Property->IsA<FArrayProperty>()
		|| Property->IsA<FSetProperty>()
		|| Property->IsA<FWeakObjectProperty>()
		|| Property->IsA<FLazyObjectProperty>()
		|| Property->IsA<FSoftObjectProperty>()
		|| Property->IsA<FSoftClassProperty>()
		|| Property->IsA<FInterfaceProperty>()
		|| Property->IsA<FDelegateProperty>()
		|| Property->IsA<FFieldPathProperty>()
		|| Property->IsA<FMulticastInlineDelegateProperty>()
		|| Property->IsA<FMulticastSparseDelegateProperty>()
		;
#else
	return true;
#endif
}

bool IsScalarProperty(FField* Property)
{
	check(Property);
	bool bIsCompound = Property->IsA<FStructProperty>()
		|| Property->IsA<FObjectProperty>()
		|| Property->IsA<FMapProperty>()
		|| Property->IsA<FArrayProperty>()
		|| Property->IsA<FSetProperty>();

	return !bIsCompound;
}

void VisitAllEffectivePropertyClass(TFunctionRef<void(FFieldClass*)> Visitor)
{
	Visitor(FBoolProperty::StaticClass());

	Visitor(FInt8Property::StaticClass());
	Visitor(FInt16Property::StaticClass());
	Visitor(FIntProperty::StaticClass());
	Visitor(FInt64Property::StaticClass());

	Visitor(FByteProperty::StaticClass());
	Visitor(FUInt16Property::StaticClass());
	Visitor(FUInt32Property::StaticClass());
	Visitor(FUInt64Property::StaticClass());

	Visitor(FFloatProperty::StaticClass());
	Visitor(FDoubleProperty::StaticClass());

	Visitor(FStrProperty::StaticClass());
	Visitor(FNameProperty::StaticClass());
	Visitor(FTextProperty::StaticClass());
	Visitor(FEnumProperty::StaticClass());
	Visitor(FStructProperty::StaticClass());
	Visitor(FClassProperty::StaticClass());
	Visitor(FObjectProperty::StaticClass());
	Visitor(FMapProperty::StaticClass());
	Visitor(FArrayProperty::StaticClass());
	Visitor(FSetProperty::StaticClass());
	Visitor(FWeakObjectProperty::StaticClass());
	Visitor(FLazyObjectProperty::StaticClass());
	Visitor(FSoftObjectProperty::StaticClass());
	Visitor(FSoftClassProperty::StaticClass());
	Visitor(FInterfaceProperty::StaticClass());
	Visitor(FDelegateProperty::StaticClass());
	Visitor(FFieldPathProperty::StaticClass());
	Visitor(FMulticastInlineDelegateProperty::StaticClass());
	Visitor(FMulticastSparseDelegateProperty::StaticClass());
}

FProperty* NextEffectiveProperty(FProperty* Property)
{
	while (true)
	{
		if (Property == nullptr)
			return nullptr;

		Property = Property->PropertyLinkNext;

		if (Property == nullptr)
			return nullptr;

		if (IsEffectiveProperty(Property))
			return Property;
	}

	checkNoEntry();
	return nullptr;
}

FProperty* FirstEffectiveProperty(FProperty* Property)
{
	if (Property == nullptr)
		return nullptr;

	return IsEffectiveProperty(Property)
		? Property
		: NextEffectiveProperty(Property);
}

FProperty* FindEffectivePropertyByName(UStruct* Struct, const FName& Name)
{
	FProperty* Property = PropertyAccessUtil::FindPropertyByName(Name, Struct);
	if (Property == nullptr)
		return nullptr;
	
	return IsEffectiveProperty(Property)
		? Property
		: nullptr;
}

FProperty* FindEffectivePropertyByOffset(UStruct* Struct, size_t Offset)
{
	FProperty* Property = Struct->PropertyLink;
	while (true)
	{
		if (Property == nullptr)
			return nullptr;

		if (!IsEffectiveProperty(Property))
			return nullptr;

		if (Property->GetOffset_ForInternal() == Offset)
			return Property;

		Property = Property->PropertyLinkNext;
	}

	return nullptr;
}

FDcResult FindEffectivePropertyByOffset(UStruct* Struct, size_t Offset, FProperty*& OutValue)
{
	OutValue = FindEffectivePropertyByOffset(Struct, Offset);
	if (OutValue == nullptr)
	{
		return DC_FAIL(DcDReadWrite, FindPropertyByOffsetFailed)
			<< Struct->GetFName() << Offset;
	}
	else
	{
		return DcOk();
	}
}

EDcDataEntry PropertyToDataEntry(FField* Property)
{
	check(Property)
	if (Property->IsA<FBoolProperty>()) return EDcDataEntry::Bool;
	if (Property->IsA<FNameProperty>()) return EDcDataEntry::Name;
	if (Property->IsA<FStrProperty>()) return EDcDataEntry::String;
	if (Property->IsA<FTextProperty>()) return EDcDataEntry::Text;
	if (Property->IsA<FEnumProperty>()) return EDcDataEntry::Enum;

	if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		if (NumericProperty->IsEnum())
		{
			return EDcDataEntry::Enum;
		}
		else
		{
			if (Property->IsA<FInt8Property>()) return EDcDataEntry::Int8;
			if (Property->IsA<FInt16Property>()) return EDcDataEntry::Int16;
			if (Property->IsA<FIntProperty>()) return EDcDataEntry::Int32;
			if (Property->IsA<FInt64Property>()) return EDcDataEntry::Int64;

			if (Property->IsA<FByteProperty>()) return EDcDataEntry::UInt8;
			if (Property->IsA<FUInt16Property>()) return EDcDataEntry::UInt16;
			if (Property->IsA<FUInt32Property>()) return EDcDataEntry::UInt32;
			if (Property->IsA<FUInt64Property>()) return EDcDataEntry::UInt64;

			if (Property->IsA<FFloatProperty>()) return EDcDataEntry::Float;
			if (Property->IsA<FDoubleProperty>()) return EDcDataEntry::Double;
		}
	}

	{
		//	order significant
		if (Property->IsA<FClassProperty>()) return EDcDataEntry::ClassReference;
		if (Property->IsA<FStructProperty>()) return EDcDataEntry::StructRoot;
	}

	if (Property->IsA<FWeakObjectProperty>()) return EDcDataEntry::WeakObjectReference;
	if (Property->IsA<FLazyObjectProperty>()) return EDcDataEntry::LazyObjectReference;

	{
		//	order significant
		if (Property->IsA<FSoftClassProperty>()) return EDcDataEntry::SoftClassReference;
		if (Property->IsA<FSoftObjectProperty>()) return EDcDataEntry::SoftObjectReference;
		if (Property->IsA<FInterfaceProperty>()) return EDcDataEntry::InterfaceReference;
		if (Property->IsA<FObjectProperty>()) return EDcDataEntry::ClassRoot;
	}

	if (Property->IsA<FFieldPathProperty>()) return EDcDataEntry::FieldPath;
	if (Property->IsA<FDelegateProperty>()) return EDcDataEntry::Delegate;
	if (Property->IsA<FMulticastInlineDelegateProperty>()) return EDcDataEntry::MulticastInlineDelegate;
	if (Property->IsA<FMulticastSparseDelegateProperty>()) return EDcDataEntry::MulticastSparseDelegate;

	if (Property->IsA<FMapProperty>()) return EDcDataEntry::MapRoot;
	if (Property->IsA<FArrayProperty>()) return EDcDataEntry::ArrayRoot;
	if (Property->IsA<FSetProperty>()) return EDcDataEntry::SetRoot;

	checkNoEntry();
	return EDcDataEntry::Ended;
}

EDcDataEntry PropertyToDataEntry(const FFieldVariant& Field)
{
	check(Field.IsValid());
	if (Field.IsUObject())
	{
		UObject* Obj = Field.ToUObjectUnsafe();
		if (Obj->IsA<UScriptStruct>())
		{
			return EDcDataEntry::StructRoot;
		}
		else if (Obj->IsA<UClass>())
		{
			return EDcDataEntry::ClassRoot;
		}
		else
		{
			checkNoEntry();
			return EDcDataEntry::Ended;
		}
	}
	else
	{
		return PropertyToDataEntry(Field.ToFieldUnsafe());
	}
}

FString GetFormatPropertyTypeName(FField* Property)
{
	check(Property);
	if (Property->IsA<FBoolProperty>()) return TEXT("bool");
	if (Property->IsA<FNameProperty>()) return TEXT("FName");
	if (Property->IsA<FTextProperty>()) return TEXT("FText");
	if (Property->IsA<FStrProperty>()) return TEXT("FString");

	if (Property->IsA<FInt8Property>()) return TEXT("int8");
	if (Property->IsA<FInt16Property>()) return TEXT("int16");
	if (Property->IsA<FIntProperty>()) return TEXT("int32");
	if (Property->IsA<FInt64Property>()) return TEXT("int64");

	if (Property->IsA<FByteProperty>()) return TEXT("uint8");
	if (Property->IsA<FUInt16Property>()) return TEXT("uint16");
	if (Property->IsA<FUInt32Property>()) return TEXT("uint32");
	if (Property->IsA<FUInt64Property>()) return TEXT("uint64");

	if (Property->IsA<FFloatProperty>()) return TEXT("float");
	if (Property->IsA<FDoubleProperty>()) return TEXT("double");

	if (Property->IsA<FFieldPathProperty>()) return ((FProperty*)Property)->GetCPPType(nullptr, 0);
	if (Property->IsA<FDelegateProperty>()) return ((FProperty*)Property)->GetCPPType(nullptr, 0);
	if (Property->IsA<FMulticastInlineDelegateProperty>()) return ((FProperty*)Property)->GetCPPType(nullptr, 0);
	if (Property->IsA<FMulticastSparseDelegateProperty>()) return ((FProperty*)Property)->GetCPPType(nullptr, 0);

	if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		return FString::Printf(TEXT("E%s"), *EnumProperty->GetEnum()->GetName());
	}

	if (FStructProperty* StructField = CastField<FStructProperty>(Property))
	{
		return FString::Printf(TEXT("F%s"), *StructField->Struct->GetName());
	}

	if (FObjectProperty* ObjField = CastField<FObjectProperty>(Property))
	{
		return FString::Printf(TEXT("U%s"), *ObjField->PropertyClass->GetName());
	}

	if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		return FString::Printf(TEXT("TMap<%s, %s>"),
			*GetFormatPropertyTypeName(MapProperty->KeyProp),
			*GetFormatPropertyTypeName(MapProperty->ValueProp)
		);
	}
	if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		return FString::Printf(TEXT("TArray<%s>"),
			*GetFormatPropertyTypeName(ArrayProperty->Inner)
		);
	}

	if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		return FString::Printf(TEXT("TSet<%s>"),
			*GetFormatPropertyTypeName(SetProperty->ElementProp)
			);
	}

	if (FWeakObjectProperty* WeakProperty = CastField<FWeakObjectProperty>(Property))
	{
		return FString::Printf(TEXT("TWeakObjectPtr<%s>"),
			*GetFormatPropertyTypeName(WeakProperty->PropertyClass)
		);
	}

	if (FLazyObjectProperty* LazyProperty = CastField<FLazyObjectProperty>(Property))
	{
		return FString::Printf(TEXT("TLazyObjectPtr<%s>"),
			*GetFormatPropertyTypeName(LazyProperty->PropertyClass)
		);
	}

	{
		//	order significant
		if (FSoftClassProperty* SoftProperty = CastField<FSoftClassProperty>(Property))
		{
			return FString::Printf(TEXT("TSoftClassPtr<%s>"),
				*GetFormatPropertyTypeName(SoftProperty->MetaClass)
			);
		}

		if (FSoftObjectProperty* SoftProperty = CastField<FSoftObjectProperty>(Property))
		{
			return FString::Printf(TEXT("TSoftObjectPtr<%s>"),
				*GetFormatPropertyTypeName(SoftProperty->PropertyClass)
			);
		}
	}

	if (FInterfaceProperty* InterfaceProperty = CastField<FInterfaceProperty>(Property))
	{
		return FString::Printf(TEXT("TScriptInterface<%s>"),
			*GetFormatPropertyTypeName(InterfaceProperty->InterfaceClass)
		);
	}

	checkNoEntry();
	return FString();
}

FString GetFormatPropertyTypeName(UScriptStruct* Struct)
{
	check(Struct);
	return FString::Printf(TEXT("F%s"), *Struct->GetName());
}

FString GetFormatPropertyTypeName(UClass* Class)
{
	check(Class);
	return FString::Printf(TEXT("U%s"), *Class->GetName());
}

FString GetFormatPropertyTypeName(const FFieldVariant& Field)
{
	check(Field.IsValid());
	if (Field.IsUObject())
	{
		UObject* Obj = Field.ToUObjectUnsafe();
		if (UScriptStruct* Struct = Cast<UScriptStruct>(Obj))
		{
			return GetFormatPropertyTypeName(Struct);
		}
		else if (UClass* Class = Cast<UClass>(Obj))
		{
			return GetFormatPropertyTypeName(Class);
		}
		else
		{
			checkNoEntry();
			return TEXT("<invalid>");
		}
	}
	else
	{
		return GetFormatPropertyTypeName(Field.ToFieldUnsafe());
	}
}

bool IsSubObjectProperty(FObjectProperty* ObjectProperty)
{
	//	check `UPROPERTY(Instanced)`
	return ObjectProperty->HasAnyPropertyFlags(CPF_InstancedReference)
	//	check UCLASS(DefaultToInstanced, EditInlineNew)
		|| ObjectProperty->PropertyClass->HasAnyClassFlags(CLASS_EditInlineNew | CLASS_DefaultToInstanced);
}

bool IsUnsignedProperty(FNumericProperty* NumericProperty)
{
	return NumericProperty->IsA<FByteProperty>()
		|| NumericProperty->IsA<FUInt16Property>()
		|| NumericProperty->IsA<FUInt32Property>()
		|| NumericProperty->IsA<FUInt64Property>();
}

FName GetStructTypeName(FFieldVariant& Property)
{
	if (!Property.IsValid())
	{
		return FName();
	}
	else if (Property.IsA<FStructProperty>())
	{
		return CastFieldChecked<FStructProperty>(Property.ToFieldUnsafe())->Struct->GetFName();
	}
	else if (Property.IsA<UScriptStruct>())
	{
		return CastChecked<UScriptStruct>(Property.ToUObjectUnsafe())->GetFName();
	}
	else
	{
		return FName();
	}
}

UScriptStruct* TryGetStructClass(FFieldVariant& FieldVariant)
{
	if (FStructProperty* StructProperty = CastFieldVariant<FStructProperty>(FieldVariant))
	{
		return StructProperty->Struct;
	}
	else if (UScriptStruct* StructClass = CastFieldVariant<UScriptStruct>(FieldVariant))
	{
		return StructClass;
	}
	else
	{
		return nullptr;
	}
}

bool TryGetEnumPropertyOut(const FFieldVariant& Field, UEnum*& OutEnum, FNumericProperty*& OutNumeric)
{
	if (FEnumProperty* EnumProperty = CastFieldVariant<FEnumProperty>(Field))
	{
		OutNumeric = EnumProperty->GetUnderlyingProperty();
		OutEnum = EnumProperty->GetEnum();
		return true;
	}
	else if (FNumericProperty* NumericProperty = CastFieldVariant<FNumericProperty>(Field))
	{
		if (NumericProperty->IsEnum())
		{
			OutNumeric = NumericProperty;
			OutEnum = OutNumeric->GetIntPropertyEnum();
			check(OutEnum);
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

FDcResult GetEnumProperty(const FFieldVariant& Field, UEnum*& OutEnum, FNumericProperty*& OutNumeric)
{
	return TryGetEnumPropertyOut(Field, OutEnum, OutNumeric)
		? DcOk()
		: DC_FAIL(DcDReadWrite, PropertyMismatch2)
			<< TEXT("EnumProperty")  << TEXT("<NumericProperty with Enum>")
			<< Field.GetFName() << Field.GetClassName();
}

UStruct* TryGetStruct(const FFieldVariant& FieldVariant)
{
	if (FieldVariant.IsA<UScriptStruct>())
	{
		return (UScriptStruct*)FieldVariant.ToUObjectUnsafe();
	}
	else if (FieldVariant.IsA<UClass>())
	{
		return (UClass*)FieldVariant.ToUObjectUnsafe();
	}
	else if (FieldVariant.IsA<FStructProperty>())
	{
		return CastFieldChecked<FStructProperty>(FieldVariant.ToFieldUnsafe())->Struct;
	}
	else if (FieldVariant.IsA<FObjectProperty>())
	{
		return CastFieldChecked<FObjectProperty>(FieldVariant.ToFieldUnsafe())->PropertyClass;
	}
	else
	{
		return nullptr;
	}
}

UStruct* TryGetStruct(const FDcPropertyDatum& Datum)
{
	return TryGetStruct(Datum.Property);
}
}	// namespace DcPropertyUtils