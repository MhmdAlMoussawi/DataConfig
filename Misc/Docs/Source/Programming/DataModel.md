# DataConfig Data Model

Conceptually the DataConfig data model is defined by 3 C++ types:

1. `EDcDataEntry` - enum covers every possible data type.
2. `FDcReader` - methods to read from the data model.
3. `FDcWriter` - methods to write into the data model.

And that's it. The obvious missing thing is a DOM like object that you can random access and serialize into -  we choose to not implement that and it's crucial to understand this to get to know how DataConfig works.

## `EDcDataEntry`

The enum covers all possible types:

```c++
// DataConfigCore/Public/DataConfig/DcTypes.h
UENUM()
enum class EDcDataEntry : uint16
{
    None,

    Bool,
    Name,
    String,
    Text,
    Enum,

    Float,
    Double,

    Int8,
    Int16,
    Int32,
    Int64,

    UInt8,
    UInt16,
    UInt32,
    UInt64,

    //  Struct
    StructRoot,
    StructEnd,

    //  Class
    ClassRoot,
    ClassEnd,

    //  Map
    MapRoot,
    MapEnd,

    //  Array
    ArrayRoot,
    ArrayEnd,

    //  Set,
    SetRoot,
    SetEnd,

    //  Optional
    OptionalRoot,
    OptionalEnd,

    //  Reference
    ObjectReference,
    ClassReference,

    WeakObjectReference,
    LazyObjectReference,
    SoftObjectReference,
    SoftClassReference,
    InterfaceReference,

    //  Delegates
    Delegate,
    MulticastInlineDelegate,
    MulticastSparseDelegate,

    //  Field
    FieldPath,

    //  Extra
    Blob,

    //  Extension
    Extension,

    //  End
    Ended,
};
```

Most enumerators directly maps to a `FProperty` type:

- `EDcDataEntry::Bool`  - `FBoolProperty`
- `EDcDataEntry::Name` - `FNameProperty`
- `EDcDataEntry::String` - `FStrProperty`
- `EDcDataEntry::ArrayRoot/ArrayEnd`- `FArrayProperty`

It should've covered all possible `FProperty` types. There're some additions that has no direct `FProperty` mapping:

* `EDcDataEntry::None` -  It's maps `null` in JSON, and it's also used to explicitly represent null object reference.
* `EDcDataEntry::Ended` - It's a phony type that is returned when there's no more data or reader/writer is in a invalid state.
* `EDcDataEntry::Blob` - It's an extension to allow direct memory read/write from given fields. 
* `EDcDataEntry::Extension` - It's an extension that allows additional data formats. MsgPack reader/writer uses this to support its `extension` data types.

## `FDcReader`

`FDcReader` is the one and only way to read from DataConfig data model. For every enumerator in `EDcDataEntry` there's a member method on `FDcReader` to from it.

Here we set up a simple struct trying out the reader methods:

```c++
// DataConfigTests/Private/DcTestBlurb.h
USTRUCT()
struct FDcTestExampleSimple
{
	GENERATED_BODY()

	UPROPERTY() FString StrField;
	UPROPERTY() int IntField;
};

// DataConfigTests/Private/DcTestBlurb.cpp
FDcTestExampleSimple SimpleStruct;
SimpleStruct.StrField = TEXT("Foo Str");
SimpleStruct.IntField = 253;
```

Since we know exactly how the `FDcTestExampleSimple` looks like we can manually arrange the read calls:

```c++
// DataConfigTests/Private/DcTestBlurb.cpp
FDcPropertyReader Reader{FDcPropertyDatum(&SimpleStruct)};

DC_TRY(Reader.ReadStructRoot(&Struct));   // `FDcTestExampleSimple` Struct Root

    DC_TRY(Reader.ReadName(&FieldName));  // 'StrField' as FName
    DC_TRY(Reader.ReadString(&StrValue)); // "Foo STr"

    DC_TRY(Reader.ReadName(&FieldName));  // 'IntField' as FName
    DC_TRY(Reader.ReadInt32(&IntValue));  // 253

DC_TRY(Reader.ReadStructEnd(&Struct));    // `FDcTestExampleSimple` Struct Root
```

In the example above `FDcReader` behave like a iterator as each `ReadXXX()` call emits value and move the internal cursor into the next slot. In case we're reading a unknown structure, we can use `FReader::PeekRead()` to peek what's coming next.

## `FDcWriter`

`FDcWriter` is the counter part of writing into the data config model. To write into the example instance above:

```c++
DC_TRY(Writer.WriteStructRoot(FDcStructStat{})); // `FDcTestExampleSimple` Struct Root

    DC_TRY(Writer.WriteName(TEXT("StrField")));      // 'StrField' as FName
    DC_TRY(Writer.WriteString(TEXT("Alt Str")));     // "Foo STr"

    DC_TRY(Writer.WriteName(TEXT("IntField")));      // 'IntField' as FName
    DC_TRY(Writer.WriteInt32(233));                  // 233

DC_TRY(Writer.WriteStructEnd(FDcStructStat{}));  // `FDcTestExampleSimple` Struct Root
```

There's also `FDcWriter::PeekRead()` to query whether it's possible to write given data type.

## Sum Up

DataConfig provide `FDcReader` and `FDcWriter` to access the property system. It can be considered as a friendly alternative to the property system API. This is also how we support [JSON][1] and [MsgPack][2] in an uniform API.

[1]: ../Formats/JSON.md "JSON"
[2]: ../Formats/MsgPack.md "MsgPack"



