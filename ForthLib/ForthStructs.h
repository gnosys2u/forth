#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthStructs.h: support for user-defined structures
//
//////////////////////////////////////////////////////////////////////

#include "Forth.h"
#include "ForthBuiltinClasses.h"
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
class ForthObjectReader;

// each new structure type definition is assigned a unique index
// the struct type index is:
// - recorded in the type info field of vocabulary entries for global struct instances
// - recorded in the type info field of struct vocabulary entries for members of this type
// - used to get from a struct field to the struct vocabulary which defines its subfields

struct ForthTypeInfo
{
	ForthTypeInfo()
		: pVocab(NULL)
		, op(OP_ABORT)
		, typeIndex(static_cast<int>(kBCIInvalid))
	{}

	ForthTypeInfo(ForthStructVocabulary* inVocab, forthop inOp, int inTypeIndex)
		: pVocab(inVocab)
		, op(inOp)
		, typeIndex(inTypeIndex)
	{}

    ForthStructVocabulary*  pVocab;
    forthop                 op;
	int                     typeIndex;
};

typedef struct
{
    forthop*                    pMethods;
    ucell                       refCount;
	ForthClassVocabulary*       pVocab;
    forthop                     newOp;
} ForthClassObject;

typedef bool(*CustomObjectReader)(const std::string& elementName, ForthObjectReader* reader);

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
	int32_t offset;
	int32_t len;
	int32_t typeIndex;
	int32_t numElements;
} ForthFieldInitInfo;

class ForthInterface
{
public:
	ForthInterface( ForthClassVocabulary* pDefiningClass=NULL );
	virtual ~ForthInterface();

	void					Copy( ForthInterface* pInterface, bool isPrimaryInterface );
	void					Implements( ForthClassVocabulary* pClass );
	ForthClassVocabulary*	GetDefiningClass();
	forthop*				GetMethods();
    forthop					GetMethod( int index );
	void					SetMethod(int index, forthop method );
    int					    AddMethod(forthop method );
    int                     GetMethodIndex( const char* pName );
    int					    GetNumMethods();
    int					    GetNumAbstractMethods();
protected:
	ForthClassVocabulary*   mpDefiningClass;
    std::vector<forthop>    mMethods;
	int                     mNumAbstractMethods;
};

// a struct accessor compound operation must be less than this length in longs
#define MAX_ACCESSOR_LONGS  64

class ForthTypesManager : public ForthForgettable
{
public:
    ForthTypesManager();
    ~ForthTypesManager();

    virtual void    ForgetCleanup( void *pForgetLimit, forthop op );

    // compile/interpret symbol if it is a valid structure accessor
    virtual bool    ProcessSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus );

    // compile symbol if it is a class member variable or method
    virtual bool    ProcessMemberSymbol( ForthParseInfo *pInfo, eForthResult& exitStatus );

    void            AddBuiltinClasses( ForthEngine* pEngine );
    void            ShutdownBuiltinClasses(ForthEngine* pEngine);

    // add a new structure type
    ForthStructVocabulary*          StartStructDefinition( const char *pName );
    void                            EndStructDefinition( void );
	// default classIndex value means assign next available classIndex
	ForthClassVocabulary*           StartClassDefinition(const char *pName, int classIndex = kNumBuiltinClasses);
    void                            EndClassDefinition( void );
    static ForthTypesManager*       GetInstance( void );

    // return info structure for struct type specified by typeIndex
    ForthTypeInfo*        GetTypeInfo( int typeIndex );
	ForthClassVocabulary* GetClassVocabulary(int typeIndex) const;
	ForthInterface* GetClassInterface(int typeIndex, int interfaceIndex) const;

    // return vocabulary for a struct type given its opcode or name
    ForthStructVocabulary*  GetStructVocabulary( forthop op );
	ForthStructVocabulary*	GetStructVocabulary( const char* pName );

    void GetFieldInfo( int32_t fieldType, int32_t& fieldBytes, int32_t& alignment );

    ForthStructVocabulary*  GetNewestStruct( void );
    ForthClassVocabulary*   GetNewestClass( void );
    forthBaseType           GetBaseTypeFromName( const char* typeName );
    ForthNativeType*        GetNativeTypeFromName( const char* typeName );
    int32_t                    GetBaseTypeSizeFromName( const char* typeName );
    forthop*                GetClassMethods();

    virtual const char* GetTypeName();
    virtual const char* GetName();

	inline const std::vector<ForthFieldInitInfo>&	GetFieldInitInfos() {  return mFieldInitInfos;  }
	void AddFieldInitInfo(const ForthFieldInitInfo& fieldInitInfo);
	void					DefineInitOpcode();

protected:
    // mStructInfo is an array with an entry for each defined structure type
	std::vector<ForthTypeInfo>      mStructInfo;
    static ForthTypesManager*       mpInstance;
    ForthVocabulary*                mpSavedDefinitionVocab;
    char                            mToken[ DEFAULT_INPUT_BUFFER_LEN ];
    forthop                         mCode[ MAX_ACCESSOR_LONGS ];
    forthop*                        mpClassMethods;
	ForthStructCodeGenerator*		mpCodeGenerator;
	std::vector<ForthFieldInitInfo>	mFieldInitInfos;
	cell							mNewestTypeIndex;
};

class ForthStructVocabulary : public ForthVocabulary
{
public:
    ForthStructVocabulary( const char* pName, int typeIndex );
    virtual ~ForthStructVocabulary();

    // return pointer to symbol entry, NULL if not found
    virtual forthop*    FindSymbol( const char *pSymName, ucell serial=0 );

    // delete symbol entry and all newer entries
    // return true IFF symbol was forgotten
    virtual bool        ForgetSymbol( const char   *pSymName );

    // forget all ops with a greater op#
    virtual void        ForgetOp( forthop op );

    virtual const char* GetTypeName();

    virtual const char* GetType( void );

    virtual void        PrintEntry(forthop*   pEntry);
    static void         TypecodeToString( int32_t typeCode, char* outBuff, size_t outBuffSize );

    // handle invocation of a struct op - define a local/global struct or struct array, or define a field
    virtual void	    DefineInstance( void );

	virtual bool		IsStruct( void );

    void                AddField( const char* pName, int32_t fieldType, int numElements );
    int                 GetAlignment( void );
    int                 GetSize( void );
    void                StartUnion( void );
    virtual void        Extends( ForthStructVocabulary *pParentStruct );

    inline ForthStructVocabulary* BaseVocabulary( void ) { return mpSearchNext; }

    inline int32_t         GetTypeIndex( void ) { return mTypeIndex; };

    virtual void        EndDefinition();

    virtual void		ShowData(const void* pData, ForthCoreState* pCore, bool showId);
    // returns number of top-level data items shown
    // pass optional pEndVocab to prevent showing items from that vocab or lower
    virtual int		    ShowDataInner(const void* pData, ForthCoreState* pCore,
        ForthStructVocabulary* pEndVocab = nullptr);

	inline forthop			GetInitOpcode() { return mInitOpcode;  }
	void				SetInitOpcode(forthop op);

protected:
    int                     mNumBytes;
    int                     mMaxNumBytes;
    int                     mTypeIndex;
    int                     mAlignment;
    ForthStructVocabulary   *mpSearchNext;
	forthop					mInitOpcode;
};

class ForthClassVocabulary : public ForthStructVocabulary
{
public:
    ForthClassVocabulary( const char* pName, int typeIndex );
    virtual ~ForthClassVocabulary();

    // handle invocation of a struct op - define a local/global struct or struct array, or define a field
    virtual void	    DefineInstance(void);
    virtual void	    DefineInstance(const char* pInstanceName, const char* pContainedClassName = nullptr);

    virtual const char* GetTypeName();

	int                 AddMethod( const char* pName, int methodIndex, forthop op );
	int 				FindMethod( const char* pName );
	void				Implements( const char* pName );
	void				EndImplements( void );
	int32_t				GetClassId( void )		{ return mTypeIndex; }

	ForthInterface*		GetInterface( int32_t index );
    forthop*            GetMethods();
    int32_t                FindInterfaceIndex( int32_t classId );
	virtual bool		IsClass( void );
	int32_t				GetNumInterfaces( void );
    virtual void        Extends( ForthClassVocabulary *pParentClass );
    ForthClassObject*   GetClassObject(void);
    void                FixClassObjectMethods(void);
    ForthClassVocabulary* ParentClass( void );

    virtual void        PrintEntry(forthop*   pEntry);
    void                SetCustomObjectReader(CustomObjectReader reader);
    CustomObjectReader  GetCustomObjectReader();

protected:
    int32_t                        mCurrentInterface;
	ForthClassVocabulary*       mpParentClass;
	std::vector<ForthInterface *>	mInterfaces;
    ForthClassObject*           mpClassObject;
    CustomObjectReader          mCustomReader;
	static ForthClassVocabulary* smpObjectClass;
};

class ForthNativeType
{
public:
    ForthNativeType( const char* pName, int numBytes, forthBaseType nativeType );
    virtual ~ForthNativeType();
    virtual void DefineInstance( ForthEngine *pEngine, void *pInitialVal, int32_t flags=0 );

    inline int32_t GetGlobalOp( void ) { return mBaseType + gCompiledOps[OP_DO_BYTE]; };
    inline int32_t GetGlobalArrayOp( void ) { return mBaseType + gCompiledOps[OP_DO_BYTE_ARRAY]; };
    inline int32_t GetLocalOp( void ) { return mBaseType + kOpLocalByte; };
    inline int32_t GetFieldOp( void ) { return mBaseType + kOpFieldByte; };
    inline int32_t GetAlignment( void ) { return (mNumBytes > 4) ? 4 : mNumBytes; };
    inline int32_t GetSize( void ) { return mNumBytes; };
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

