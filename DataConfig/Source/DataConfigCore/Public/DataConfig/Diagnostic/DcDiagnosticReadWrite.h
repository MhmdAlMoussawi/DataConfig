#pragma once

#include "HAL/Platform.h"

namespace DcDReadWrite
{

static const uint16 Category = 0x2;

enum Type : uint16
{
	Unknown = 0,
	InvalidStateNoExpect,
	InvalidStateWithExpect,
	InvalidStateWithExpect2,
	DataTypeMismatch,
	DataTypeMismatch2,
	PropertyMismatch,
	PropertyMismatch2,
	AlreadyEnded,
	CantFindPropertyByName,
	WriteClassInlineNotCreated,
	StructNameMismatch,
	ClassNameMismatch,
	EnumNameMismatch,
	EnumNameNotFound,
	EnumValueInvalid,
	EnumSignMismatch,
	WriteBlobOverrun,
	FindPropertyByOffsetFailed,
	DataTypeUnequal,
	DataTypeUnequalLhsRhs,
	ExpectFieldButFoundUObject,
	
	//	putback reader
	CantUsePutbackValue,

	//	pipe visitor
	PipeReadWriteMismatch,

	//	skip
	SkipOutOfRange,

};

} // namespace DcDReadWrite
