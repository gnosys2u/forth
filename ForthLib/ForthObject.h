#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthObject.h: forth base object definitions
//
//////////////////////////////////////////////////////////////////////

#define METHOD_RETURN     SET_TP((ForthObject) (RPOP))

#define RPUSH_OBJECT( _object ) RPUSH( ((long) (_object)))
#define RPUSH_THIS  RPUSH_OBJECT( GET_TP )
#define SET_THIS( _object) SET_TP( (_object) )

#define METHOD( NAME, VALUE  )          { NAME, (ulong) VALUE, NATIVE_TYPE_TO_CODE( kDTIsMethod, kBaseTypeVoid ) }
#define METHOD_RET( NAME, VAL, RVAL )   { NAME, (ulong) VAL, RVAL }
#define MEMBER_VAR( NAME, TYPE )        { NAME, 0, (ulong) TYPE }
#define MEMBER_ARRAY( NAME, TYPE, NUM ) { NAME, NUM, (ulong) (TYPE | kDTIsArray) }
#define CLASS_OP( NAME, VALUE )         { NAME, (ulong) VALUE, NATIVE_TYPE_TO_CODE(0, kBaseTypeUserDefinition) }
#define CLASS_PRECOP( NAME, VALUE )     { NAME, (ulong) VALUE, NATIVE_TYPE_TO_CODE(kDTIsFunky, kBaseTypeUserDefinition) }

#define END_MEMBERS { NULL, 0, 0 }

#define FULLY_EXECUTE_METHOD( _pCore, _obj, _methodNum ) ForthEngine::GetInstance()->FullyExecuteMethod( _pCore, _obj, _methodNum )

#define PUSH_OBJECT( _obj )             SPUSH((long)(_obj))
#define POP_OBJECT( _obj )              _obj = (ForthObject)(SPOP)

#define GET_THIS( THIS_TYPE, THIS_NAME ) THIS_TYPE* THIS_NAME = reinterpret_cast<THIS_TYPE *>(GET_TP);

#define SAFE_RELEASE( _pCore, _obj ) \
	if ( _obj != nullptr ) { \
		_obj->refCount -= 1; \
		if ( _obj->refCount == 0 ) { FULLY_EXECUTE_METHOD( (_pCore), (_obj), kMethodDelete ); } \
	} TRACK_RELEASE

#define SAFE_KEEP( _obj )       if ( _obj != nullptr ) { _obj->refCount += 1; } TRACK_KEEP

#define OBJECTS_DIFFERENT( OLDOBJ, NEWOBJ ) (OLDOBJ != NEWOBJ)
#define OBJECTS_SAME( OLDOBJ, NEWOBJ ) (OLDOBJ == NEWOBJ)

#define CLEAR_OBJECT( _obj )             (_obj) = nullptr

#define OBJECT_ASSIGN( _pCore, _dstObj, _srcObj ) if ( (_dstObj) != (_srcObj) ) { SAFE_KEEP( (_srcObj) ); SAFE_RELEASE( (_pCore), (_dstObj) ); }

enum
{
    // all objects have methods 0..6
    kMethodDelete,
    kMethodShow,
    kMethodShowInner,
    kMethodGetClass,
    kMethodCompare,
	kMethodKeep,
	kMethodRelease,
	kNumBaseMethods
};


// debug tracking of object allocations

#define TRACK_OBJECT_ALLOCATIONS
#ifdef TRACK_OBJECT_ALLOCATIONS
extern long gStatNews;
extern long gStatDeletes;
extern long gStatLinkNews;
extern long gStatLinkDeletes;
extern long gStatIterNews;
extern long gStatIterDeletes;
extern long gStatKeeps;
extern long gStatReleases;

#define TRACK_NEW			gStatNews++
#define TRACK_DELETE		gStatDeletes++
#define TRACK_LINK_NEW		gStatLinkNews++
#define TRACK_LINK_DELETE	gStatLinkDeletes++
#define TRACK_ITER_NEW		gStatIterNews++
#define TRACK_ITER_DELETE	gStatIterDeletes++
#define TRACK_KEEP			gStatKeeps++
#define TRACK_RELEASE		gStatReleases++
#else
#define TRACK_NEW
#define TRACK_DELETE
#define TRACK_LINK_NEW
#define TRACK_LINK_DELETE
#define TRACK_ITER_NEW
#define TRACK_ITER_DELETE
#define TRACK_KEEP
#define TRACK_RELEASE
#endif

#define MALLOCATE( _type, _ptr ) _type* _ptr = (_type *) __MALLOC( sizeof(_type) );

#define MALLOCATE_OBJECT( _type, _ptr, _vocab )   _type* _ptr = (_type *) __MALLOC( _vocab->GetSize() );  TRACK_NEW
#define FREE_OBJECT( _obj )  __FREE( _obj );  TRACK_DELETE
#define MALLOCATE_LINK( _type, _ptr )  MALLOCATE( _type, _ptr );  TRACK_LINK_NEW
#define FREE_LINK( _link )  __FREE( _link );  TRACK_LINK_DELETE
#define MALLOCATE_ITER( _type, _ptr, _vocab )  MALLOCATE_OBJECT( _type, _ptr, _vocab );  TRACK_ITER_NEW
#define FREE_ITER( _link )  FREE_OBJECT( _link );  TRACK_ITER_DELETE

// UNDELETABLE_OBJECT_REFCOUNT is used for objects like the system object or vocabularies which
//   you don't want to be mistakenly deleted due to refcount mistakes
#define UNDELETABLE_OBJECT_REFCOUNT 2000000000