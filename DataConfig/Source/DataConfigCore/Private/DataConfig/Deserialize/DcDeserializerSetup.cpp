#include "DataConfig/Deserialize/DcDeserializerSetup.h"
#include "DataConfig/Property/DcPropertyUtils.h"
#include "DataConfig/Deserialize/DcDeserializer.h"
#include "DataConfig/Deserialize/DcDeserializeTypes.h"
#include "DataConfig/Deserialize/Handlers/Json/DcJsonPrimitiveDeserializers.h"
#include "DataConfig/Deserialize/Handlers/Json/DcJsonStructDeserializers.h"
#include "DataConfig/Deserialize/Handlers/Json/DcJsonClassDeserializers.h"
#include "DataConfig/Deserialize/Handlers/Json/DcJsonContainerDeserializers.h"
#include "DataConfig/Deserialize/Handlers/Property/DcPropertyPipeDeserializers.h"
#include "Templates/Casts.h"
#include "UObject/UnrealType.h"
#include "UObject/TextProperty.h"

void DcSetupJsonDeserializeHandlers(FDcDeserializer& Deserializer)
{
	using namespace DcJsonHandlers;

	//	Primitives
	Deserializer.AddDirectHandler(FBoolProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerBoolDeserialize));
	Deserializer.AddDirectHandler(FNameProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerNameDeserialize));
	Deserializer.AddDirectHandler(FStrProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerStringDeserialize));
	Deserializer.AddDirectHandler(FTextProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerTextDeserialize));

	{
		// order significant
		Deserializer.AddPredicatedHandler(
			FDcDeserializePredicate::CreateStatic(PredicateIsEnumProperty),
			FDcDeserializeDelegate::CreateStatic(HandlerEnumDeserialize)
		);

		Deserializer.AddPredicatedHandler(
			FDcDeserializePredicate::CreateStatic(PredicateIsNumericProperty),
			FDcDeserializeDelegate::CreateStatic(HandlerNumericDeserialize)
		);
	}

	//	Containers
	Deserializer.AddDirectHandler(FArrayProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerArrayDeserialize));
	Deserializer.AddDirectHandler(FSetProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerSetDeserialize));
	Deserializer.AddDirectHandler(FMapProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerMapDeserialize));

	//	Struct
	Deserializer.AddDirectHandler(UScriptStruct::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerStructRootDeserialize));
	Deserializer.AddDirectHandler(FStructProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerStructRootDeserialize));

	//	Class
	Deserializer.AddDirectHandler(UClass::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerClassRootDeserialize));
	Deserializer.AddPredicatedHandler(
		FDcDeserializePredicate::CreateStatic(PredicateIsSubObjectProperty),
		FDcDeserializeDelegate::CreateStatic(HandlerInstancedSubObjectDeserialize)
	);

	FObjectReferenceHandlerGenerator ObjHandlerGen(&TryReadObjectReference);
	Deserializer.AddDirectHandler(FObjectProperty::StaticClass(), ObjHandlerGen.MakeObjectReferenceHandler());
	Deserializer.AddDirectHandler(FSoftObjectProperty::StaticClass(), ObjHandlerGen.MakeSoftObjectReferenceHandler());
	Deserializer.AddDirectHandler(FWeakObjectProperty::StaticClass(), ObjHandlerGen.MakeWeakObjectReferenceHandler());
	Deserializer.AddDirectHandler(FLazyObjectProperty::StaticClass(), ObjHandlerGen.MakeLazyObjectReferenceHandler());

	Deserializer.AddDirectHandler(FClassProperty::StaticClass(), ObjHandlerGen.MakeClassReferenceHandler());
	Deserializer.AddDirectHandler(FSoftClassProperty::StaticClass(), ObjHandlerGen.MakeSoftClassReferenceHandler());
}

void DATACONFIGCORE_API DcSetupPropertyPipeDeserializeHandlers(FDcDeserializer& Deserializer)
{
	using namespace DcPropertyPipeHandlers;

	Deserializer.AddDirectHandler(UClass::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerClassDeserialize));
	Deserializer.AddDirectHandler(FObjectProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerClassDeserialize));

	Deserializer.AddDirectHandler(UScriptStruct::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerStructDeserialize));
	Deserializer.AddDirectHandler(FStructProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerStructDeserialize));

	Deserializer.AddDirectHandler(FArrayProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerArrayDeserialize));
	Deserializer.AddDirectHandler(FSetProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerSetDeserialize));
	Deserializer.AddDirectHandler(FMapProperty::StaticClass(), FDcDeserializeDelegate::CreateStatic(HandlerMapDeserialize));

	DcPropertyUtils::VisitAllEffectivePropertyClass([&](FFieldClass* FieldClass) {
		if (!Deserializer.FieldClassDeserializerMap.Contains(FieldClass))
			Deserializer.AddDirectHandler(FieldClass, FDcDeserializeDelegate::CreateStatic(HandlerScalarDeserialize));
	});
}
