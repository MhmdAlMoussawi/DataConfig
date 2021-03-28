#include "DataConfig/Deserialize/DcDeserializer.h"
#include "DataConfig/Diagnostic/DcDiagnosticDeserialize.h"
#include "DataConfig/Diagnostic/DcDiagnosticUtils.h"
#include "DataConfig/Diagnostic/DcDiagnosticCommon.h"
#include "DataConfig/Misc/DcTemplateUtils.h"
#include "DataConfig/Property/DcPropertyWriter.h"
#include "Misc/ScopeExit.h"

namespace DcDeserializerDetails
{

static FDcResult ExecuteDeserializeHandler(FDcDeserializeContext& Ctx, FDcDeserializeDelegate& Handler)
{
	if (!Handler.IsBound())
		return DC_FAIL(DcDCommon, StaleDelegate);

	return Handler.Execute(Ctx);
}

static FDcResult DeserializeBody(FDcDeserializer& Self, FDcDeserializeContext& Ctx)
{
	//	use predicated deserializers first, if it's not handled then try direct handlers
	for (auto& PredPair : Self.PredicatedDeserializers)
	{
		if (!PredPair.Key.IsBound())
			return DC_FAIL(DcDCommon, StaleDelegate);

		if (PredPair.Key.Execute(Ctx) == EDcDeserializePredicateResult::Process)
			return ExecuteDeserializeHandler(Ctx, PredPair.Value);
	}

	FFieldVariant& Property = Ctx.TopProperty();
	FDcDeserializeDelegate* HandlerPtr;
	if (Property.IsUObject())
	{
		UObject* Object = CastChecked<UObject>(Property.ToUObjectUnsafe());
		check(IsValid(Object));
		UClass* Class = Object->GetClass();
		HandlerPtr = Self.UClassDeserializerMap.Find(Class);
		if (HandlerPtr == nullptr)
			return DC_FAIL(DcDDeserialize, NoMatchingHandler)
				<< Ctx.TopProperty().GetFName() << Class->GetFName();
	}
	else
	{
		FField* Field = Property.ToFieldUnsafe();
		check(Field->IsValidLowLevel());
		FFieldClass* FieldClass = Field->GetClass();
		HandlerPtr = Self.FieldClassDeserializerMap.Find(FieldClass);
		if (HandlerPtr == nullptr)
			return DC_FAIL(DcDDeserialize, NoMatchingHandler)
				<< Ctx.TopProperty().GetFName() << FieldClass->GetFName();
	}

	return ExecuteDeserializeHandler(Ctx, *HandlerPtr);
}

}	// namespace DcDeserializerDetails

FDcResult FDcDeserializer::Deserialize(FDcDeserializeContext& Ctx)
{
	using ECtxState = FDcDeserializeContext::EState;
	if (Ctx.State == ECtxState::Uninitialized)
	{
		return DC_FAIL(DcDDeserialize, NotPrepared);
	}
	else if (Ctx.State == ECtxState::Ready)
	{
		Ctx.State = FDcDeserializeContext::EState::DeserializeInProgress;

		ON_SCOPE_EXIT
		{
			Ctx.State = ECtxState::DeserializeEnded;
		};

		FDcResult Result = DcDeserializerDetails::DeserializeBody(*this, Ctx);
		if (!Result.Ok())
			DcDiagnosticUtils::AmendDiagnostic(DcEnv().GetLastDiag(), Ctx.Reader, Ctx.Writer);

		return Result;
	}
	else if (Ctx.State == FDcDeserializeContext::EState::DeserializeInProgress)
	{
		FDcResult Result = DcDeserializerDetails::DeserializeBody(*this, Ctx);
		if (!Result.Ok())
			DcDiagnosticUtils::AmendDiagnostic(DcEnv().GetLastDiag(), Ctx.Reader, Ctx.Writer);

		return Result;
	}
	else
	{
		return DcNoEntry();
	}
}

void FDcDeserializer::AddDirectHandler(UClass* PropertyClass, FDcDeserializeDelegate&& Delegate)
{
	check(!UClassDeserializerMap.Contains(PropertyClass));
	UClassDeserializerMap.Add(PropertyClass, MoveTemp(Delegate));
}

void FDcDeserializer::AddDirectHandler(FFieldClass* PropertyClass, FDcDeserializeDelegate&& Delegate)
{
	check(!FieldClassDeserializerMap.Contains(PropertyClass));
	FieldClassDeserializerMap.Add(PropertyClass, MoveTemp(Delegate));
}

void FDcDeserializer::AddPredicatedHandler(FDcDeserializePredicate&& Predicate, FDcDeserializeDelegate&& Delegate)
{
	PredicatedDeserializers.Add(MakeTuple(MoveTemp(Predicate), MoveTemp(Delegate)));
}
