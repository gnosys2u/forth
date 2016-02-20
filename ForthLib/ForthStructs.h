#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthStructs.h: support for user-defined structures
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"
#include "ForthForgettable.h"
#include "ForthVocabulary.h"
#include "ForthStructCodeGenerator.h"
#include <vector>

class ForthEngine;
class ForthStructVocabulary;
class ForthClassVocabulary;
class ForthNativeType;
class ForthTypesManager;
class ForthStructCodeGenerator;

// each new structure type definition is assigned a unique index
// the struct type index is:
// - recorded in the type info field of vocabulary entries for global struct instances
// - recorded in the type info field of struct vocabulary entries for members of this type
// - used to get from a struct field to the struct vocabulary which defines its subfields

typedef struct
{
    ForthStructVocabulary*      pVocab;
    long                        op;
} ForthTypeInfo;

typedef struct
{
	ulong						refCount;
	ForthClassVocabulary*       pVocab;
    long                        newOp;
} ForthClassObject;

///////////////////////////////////////

// these are the types of struct/object fields which need to be set by struct init ops
enum eForthStructInitType
{
	kFSITSuper,
	kFSITString,
	kFSITStruct,
	kFSITStringArray,
	kFSITStructArray,
};

typedef struct
{
	eForthStructInitType fieldType;
	long offset;
	long len;
	long structIndex;
	long numElements;
} ForthFieldInitInfo;

class ForthInterface
{
public:
	ForthInterface( ForthClassVocabulary* pDefiningClass=NULL );
	virtual ~ForthInterface();

	void					Copy( ForthInterface* pInterface, bool isPrimaryInterface );
	void					Implements( ForthClassVocabulary* pClass );
	ForthClassVocabulary*	GetDefiningClass();
	long*					GetMethods();
	long					GetMethod( long index );
	void					SetMethod( long index, long method );
	long					AddMethod( long method );
    long                    GetMethodIndex( const char* pName );
	long					GetNumMethods();
	long					GetNumAbstractMethods();
protected:
	ForthClassVocabulary*	mpDefiningClass;
    std::vector<long>		mMethods;
	long					mNumAbstractMethods;
};

// a struct accessor compound operation must be less than this length in longs
#define MAX_ACCESSOR_LONGS  64

class ForthTypesManager : public ForthForgettable
{
public:
    ForthTypesManager();
    ~ForthTypesManager();

    virtual void    ForgetCleanup( void *pForgetLimit, long op );

    // compile/interpret symbol if it is a valid structure accessor
    virtual bool    ProcessSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus );

    // compile symbol if it is a class member variable or method
    virtual bool    ProcessMemberSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus );

    void            AddBuiltinClasses( ForthEngine* pEngine );

    // add a new structure type
    ForthStructVocabulary*          StartStructDefinition( const char *pName );
    void                            EndStructDefinition( void );
    ForthClassVocabulary*           StartClassDefinition( const char *pName );
    void                            EndClassDefinition( void );
    static ForthTypesManager*       GetInstance( void );

    // return info structure for struct type specified by structIndex
    ForthTypeInfo*        GetStructInfo( int structIndex );

    // return vocabulary for a struct type given its opcode or name
    ForthStructVocabulary*  GetStructVocabulary( long op );
	ForthStructVocabulary*	GetStructVocabulary( const char* pName );

    void GetFieldInfo( long fieldType, long& fieldBytes, long& alignment );

    ForthStructVocabulary*  GetNewestStruct( void );
    ForthClassVocabulary*   GetNewestClass( void );
    forthBaseType           GetBaseTypeFromName( const char* typeName );
    ForthNativeType*        GetNativeTypeFromName( const char* typeName );
    long                    GetBaseTypeSizeFromName( const char* typeName );
    long*                   GetClassMethods();

    virtual const char* GetTypeName();
    virtual const char* GetName();

	inline const std::vector<ForthFieldInitInfo>&	GetFieldInitInfos() {  return mFieldInitInfos;  }
	void AddFieldInitInfo(const ForthFieldInitInfo& fieldInitInfo);
	void					DefineInitOpcode();

protected:
    // mpStructInfo points to an array with an entry for each defined structure type
    ForthTypeInfo                   *mpStructInfo;
    int                             mNumStructs;
    int                             mMaxStructs;
    static ForthTypesManager*       mpInstance;
    ForthVocabulary*                mpSavedDefinitionVocab;
    char                            mToken[ DEFAULT_INPUT_BUFFER_LEN ];
    long                            mCode[ MAX_ACCESSOR_LONGS ];
    long*                           mpClassMethods;
	ForthStructCodeGenerator*		mpCodeGenerator;
	std::vector<ForthFieldInitInfo>	mFieldInitInfos;
};

class ForthStructVocabulary : public ForthVocabulary
{
public:
    ForthStructVocabulary( const char* pName, int typeIndex );
    virtual ~ForthStructVocabulary();

    // return pointer to symbol entry, NULL if not found
    virtual long *      FindSymbol( const char *pSymName, ulong serial=0 );

    // delete symbol entry and all newer entries
    // return true IFF symbol was forgotten
    virtual bool        ForgetSymbol( const char   *pSymName );

    // forget all ops with a greater op#
    virtual void        ForgetOp( long op );

    virtual const char* GetTypeName();

    virtual const char* GetType( void );

    virtual void        PrintEntry( long*   pEntry );
    static void         TypecodeToString( long typeCode, char* outBuff, size_t outBuffSize );

    // handle invocation of a struct op - define a local/global struct or struct array, or define a field
    virtual void	    DefineInstance( void );

	virtual bool		IsStruct( void );

    void                AddField( const char* pName, long fieldType, int numElements );
    long                GetAlignment( void );
    long                GetSize( void );
    void                StartUnion( void );
    virtual void        Extends( ForthStructVocabulary *pParentStruct );

    inline ForthStructVocabulary* BaseVocabulary( void ) { return mpSearchNext; }

    inline long         GetTypeIndex( void ) { return mStructIndex; };

    virtual void        EndDefinition();

	virtual void		ShowData(const void* pData, ForthCoreState* pCore);

	inline long			GetInitOpcode() { return mInitOpcode;  }
	void				SetInitOpcode(long op);

protected:
    long                    mNumBytes;
    long                    mMaxNumBytes;
    long                    mStructIndex;
    long                    mAlignment;
    ForthStructVocabulary   *mpSearchNext;
	long					mInitOpcode;
};

class ForthClassVocabulary : public ForthStructVocabulary
{
public:
    ForthClassVocabulary( const char* pName, int typeIndex );
    virtual ~ForthClassVocabulary();

    // handle invocation of a struct op - define a local/global struct or struct array, or define a field
    virtual void	    DefineInstance( void );

    virtual void        DoOp( ForthCoreState *pCore );

    virtual const char* GetTypeName();

	long				AddMethod( const char* pName, long methodIndex, long op );
	long				FindMethod( const char* pName );
	void				Implements( const char* pName );
	void				EndImplements( void );
	long				GetClassId( void )		{ return mStructIndex; }

	ForthInterface*		GetInterface( long index );
    long                FindInterfaceIndex( long classId );
	virtual bool		IsClass( void );
	long				GetNumInterfaces( void );
    virtual void        Extends( ForthClassVocabulary *pParentClass );
    ForthClassObject*   GetClassObject( void );
    ForthClassVocabulary* ParentClass( void );

    virtual void        PrintEntry( long*   pEntry );

protected:
    long                        mCurrentInterface;
	ForthClassVocabulary*       mpParentClass;
	std::vector<ForthInterface *>	mInterfaces;
    ForthClassObject*           mpClassObject;

	static ForthClassVocabulary* smpObjectClass;
};

class ForthNativeType
{
public:
    ForthNativeType( const char* pName, int numBytes, forthBaseType nativeType );
    virtual ~ForthNativeType();
    virtual void DefineInstance( ForthEngine *pEngine, void *pInitialVal, long flags=0 );

    inline long GetGlobalOp( void ) { return mBaseType + gCompiledOps[OP_DO_BYTE]; };
    inline long GetGlobalArrayOp( void ) { return mBaseType + gCompiledOps[OP_DO_BYTE_ARRAY]; };
    inline long GetLocalOp( void ) { return mBaseType + kOpLocalByte; };
    inline long GetFieldOp( void ) { return mBaseType + kOpFieldByte; };
    inline long GetAlignment( void ) { return (mNumBytes > 4) ? 4 : mNumBytes; };
    inline long GetSize( void ) { return mNumBytes; };
    inline const char* GetName( void ) { return mpName; };
    inline forthBaseType GetBaseType( void ) { return mBaseType; };

protected:
    const char*         mpName;
    int                 mNumBytes;
    forthBaseType       mBaseType;
};

extern ForthNativeType gNativeTypeByte, gNativeTypeUByte, gNativeTypeShort, gNativeTypeUShort,
		gNativeTypeInt, gNativeTypeUInt, gNativeTypeLong, gNativeTypeULong, gNativeTypeFloat,
        gNativeTypeDouble, gNativeTypeString, gNativeTypeOp, gNativeTypeObject;

