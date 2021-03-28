#include "DataConfig/Extra/Deserialize/DcDeserializeAnyStruct.h"
#include "DataConfig/DcTypes.h"
#include "DataConfig/Reader/DcReader.h"
#include "DataConfig/Property/DcPropertyWriter.h"
#include "DataConfig/Property/DcPropertyUtils.h"
#include "DataConfig/Deserialize/DcDeserializeTypes.h"
#include "DataConfig/Deserialize/DcDeserializer.h"
#include "DataConfig/Deserialize/DcDeserializeUtils.h"
#include "DataConfig/Diagnostic/DcDiagnosticDeserialize.h"
#include "DataConfig/Diagnostic/DcDiagnosticReadWrite.h"
#include "DataConfig/DcEnv.h"
#include "DataConfig/Extra/Types/DcAnyStruct.h"
#include "DataConfig/Reader/DcPutbackReader.h"
#include "DataConfig/Automation/DcAutomation.h"
#include "DataConfig/Automation/DcAutomationUtils.h"

#include "DataConfig/Extra/Deserialize/DcDeserializeColor.h"

namespace DcExtra
{

EDcDeserializePredicateResult PredicateIsDcAnyStruct(FDcDeserializeContext& Ctx)
{
	if (FStructProperty* StructProperty = DcPropertyUtils::CastFieldVariant<FStructProperty>(Ctx.TopProperty()))
	{
		return StructProperty->Struct == FDcAnyStruct::StaticStruct()
			? EDcDeserializePredicateResult::Process
			: EDcDeserializePredicateResult::Pass;
	}
	else
	{
		return EDcDeserializePredicateResult::Pass;
	}
}

FDcResult HandlerDcAnyStructDeserialize(FDcDeserializeContext& Ctx)
{
	EDcDataEntry Next;
	DC_TRY(Ctx.Reader->PeekRead(&Next));

	FDcPropertyDatum Datum;
	DC_TRY(Ctx.Writer->WriteDataEntry(FStructProperty::StaticClass(), Datum));
	FDcAnyStruct* AnyStructPtr = (FDcAnyStruct*)Datum.DataPtr;

	if (Next == EDcDataEntry::Nil)
	{
		DC_TRY(Ctx.Reader->ReadNil());
		AnyStructPtr->Reset();

		return DcOk();
	}
	else if (Next == EDcDataEntry::MapRoot)
	{
		DC_TRY(Ctx.Reader->ReadMapRoot());
		FString Str;
		DC_TRY(Ctx.Reader->ReadString(&Str));
		if (!DcDeserializeUtils::IsMeta(Str))
			return DC_FAIL(DcDDeserialize, ExpectMetaType);

		DC_TRY(Ctx.Reader->ReadString(&Str));
		UScriptStruct* LoadStruct = FindObject<UScriptStruct>(ANY_PACKAGE, *Str, true);
		if (LoadStruct == nullptr)
			return DC_FAIL(DcDDeserialize, UObjectByStrNotFound) << TEXT("ScriptStruct") << MoveTemp(Str);

		void* DataPtr = (uint8*)FMemory::Malloc(LoadStruct->GetStructureSize());
		LoadStruct->InitializeStruct(DataPtr);
		FDcAnyStruct TmpAny{DataPtr, LoadStruct};

		*AnyStructPtr = MoveTemp(TmpAny);

		Ctx.Writer->PushTopStructPropertyState({LoadStruct, (void*)AnyStructPtr->DataPtr}, Ctx.TopProperty().GetFName());

		FDcPutbackReader PutbackReader(Ctx.Reader);
		PutbackReader.Putback(EDcDataEntry::MapRoot);
		TDcStoreThenReset<FDcReader*> RestoreReader(Ctx.Reader, &PutbackReader);

		FDcScopedProperty ScopedValueProperty(Ctx);
		DC_TRY(ScopedValueProperty.PushProperty());
		DC_TRY(Ctx.Deserializer->Deserialize(Ctx));

		return DcOk();
	}
	else
	{
		return DC_FAIL(DcDReadWrite, DataTypeMismatch2)
			<< EDcDataEntry::MapRoot << EDcDataEntry::Nil << Next;
	}
}

} // namespace DcExtra

static FDcAnyStruct _IdentityByValue(FDcAnyStruct Handle)
{
	return Handle;
}

DC_TEST("DataConfig.Extra.Deserialize.AnyStructUsage")
{
	//	instantiate from stack allocated structs
	FDcAnyStruct Any1 = new FDcExtraTestSimpleStruct1();
	Any1.GetChecked<FDcExtraTestSimpleStruct1>()->NameField = TEXT("Foo");

	//	supports moving
	FDcAnyStruct Any2 = MoveTemp(Any1);
	check(!Any1.IsValid());
	check(Any2.GetChecked<FDcExtraTestSimpleStruct1>()->NameField == TEXT("Foo"));
	Any2.Reset();

	//	supports shared referencing
	Any2 = new FDcExtraTestSimpleStruct2();
	Any2.GetChecked<FDcExtraTestSimpleStruct2>()->StrField = TEXT("Bar");

	Any1 = Any2;

	check(Any1.DataPtr == Any2.DataPtr);
	check(Any1.StructClass == Any2.StructClass);

	return true;
}

DC_TEST("DataConfig.Extra.Deserialize.AnyStructRefCounts")
{
	{
		FDcAnyStruct Alhpa(new FDcExtraTestSimpleStruct1());
		FDcAnyStruct Beta = Alhpa;

		UTEST_EQUAL("Extra AnyStruct usage", Alhpa.GetSharedReferenceCount(), 2);
	}

	{
		uint32 DestructCalledCount = 0;
		{
			FDcAnyStruct Any1(new FDcExtraTestDestructDelegateContainer());
			Any1.GetChecked<FDcExtraTestDestructDelegateContainer>()->DestructAction.BindLambda([&DestructCalledCount]{
				DestructCalledCount++;
			});

			FDcAnyStruct Any2{ Any1 };
			FDcAnyStruct Any3 = MoveTemp(Any1);
			FDcAnyStruct Any4 = _IdentityByValue(Any2);
		}

		UTEST_EQUAL("Extra AnyStruct usage", DestructCalledCount, 1);
	}

	return true;
}

DC_TEST("DataConfig.Extra.Deserialize.AnyStructDeserialize")
{
	using namespace DcExtra;
	FDcExtraTestWithAnyStruct1 Dest;
	FDcPropertyDatum DestDatum(FDcExtraTestWithAnyStruct1::StaticStruct(), &Dest);

	FString Str = TEXT(R"(

		{
			"AnyStructField1" : {
				"$type" : "DcExtraTestSimpleStruct1",
				"NameField" : "Foo"
			},
			"AnyStructField2" : {
				"$type" : "DcExtraTestStructWithColor1",
				"ColorField1" : "#0000FFFF",
				"ColorField2" : "#FF0000FF"
			},
			"AnyStructField3" : null
		}

	)");
	FDcJsonReader Reader(Str);


	UTEST_OK("Extra FAnyStruct Deserialize", DcAutomationUtils::DeserializeJsonInto(&Reader, DestDatum,
	[](FDcDeserializer& Deserializer, FDcDeserializeContext& Ctx) {
		Deserializer.AddPredicatedHandler(
			FDcDeserializePredicate::CreateStatic(PredicateIsDcAnyStruct),
			FDcDeserializeDelegate::CreateStatic(HandlerDcAnyStructDeserialize)
		);
		Deserializer.AddPredicatedHandler(
			FDcDeserializePredicate::CreateStatic(PredicateIsColorStruct),
			FDcDeserializeDelegate::CreateStatic(HandlerColorDeserialize)
		);
	}));

	UTEST_TRUE("Extra FAnyStruct Deserialize", Dest.AnyStructField1.GetChecked<FDcExtraTestSimpleStruct1>()->NameField == TEXT("Foo"));
	UTEST_TRUE("Extra FAnyStruct Deserialize", Dest.AnyStructField1.GetChecked<FDcExtraTestSimpleStruct1>()->IntFieldWithDefault == 253);
	UTEST_TRUE("Extra FAnyStruct Deserialize", Dest.AnyStructField2.GetChecked<FDcExtraTestStructWithColor1>()->ColorField1 == FColor::Blue);
	UTEST_TRUE("Extra FAnyStruct Deserialize", Dest.AnyStructField2.GetChecked<FDcExtraTestStructWithColor1>()->ColorField2 == FColor::Red);
	UTEST_TRUE("Extra FAnyStruct Deserialize", !Dest.AnyStructField3.IsValid());

	return true;
}
