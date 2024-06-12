#pragma once
//////////////////////////////////////////////////////////////////////
//
// Object.h: forth base object definitions
//
// Copyright (C) 2024 Patrick McElhatton
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////

#define METHOD_RETURN     SET_TP((ForthObject) (RPOP))

#define RPUSH_OBJECT( _object ) RPUSH( ((cell) (_object)))
#define RPUSH_THIS  RPUSH_OBJECT( GET_TP )
#define SET_THIS( _object) SET_TP( (_object) )

#define METHOD( NAME, VALUE  )          { NAME, (void *)VALUE, NATIVE_TYPE_TO_CODE( kDTIsMethod, BaseType::kVoid ) }
#define METHOD_RET( NAME, VAL, RVAL )   { NAME,  (void *)VAL, RVAL }
#define MEMBER_VAR( NAME, TYPE )        { NAME, (void *)0, (ucell) TYPE }
#define MEMBER_ARRAY( NAME, TYPE, NUM ) { NAME, NUM, (ucell) (TYPE | kDTIsArray) }
#define CLASS_OP( NAME, VALUE )         { NAME, (void *)VALUE, NATIVE_TYPE_TO_CODE(0, BaseType::kUserDefinition) }
#define CLASS_PRECOP( NAME, VALUE )     { NAME, (void *)VALUE, NATIVE_TYPE_TO_CODE(kDTIsFunky, BaseType::kUserDefinition) }

#define END_MEMBERS { nullptr, 0, 0 }

#define FULLY_EXECUTE_METHOD( _pCore, _obj, _methodNum ) ((Engine *) (_pCore->pEngine))->FullyExecuteMethod( _pCore, _obj, _methodNum )

#define PUSH_OBJECT( _obj )             SPUSH((cell)(_obj))
#define POP_OBJECT( _obj )              _obj = (ForthObject)(SPOP)

#define GET_THIS( THIS_TYPE, THIS_NAME ) THIS_TYPE* THIS_NAME = reinterpret_cast<THIS_TYPE *>(GET_TP);

#if defined(ATOMIC_REFCOUNTS)

#define SAFE_RELEASE( _pCore, _obj ) \
	if ( _obj != nullptr ) { \
		if ( _obj->refCount.fetch_sub(1) == 1 ) { ((Engine *) (_pCore->pEngine))->DeleteObject( _pCore, _obj ); } \
	} TRACK_RELEASE

#else

#define SAFE_RELEASE( _pCore, _obj ) \
	if ( _obj != nullptr ) { \
		_obj->refCount -= 1; \
		if ( _obj->refCount == 0 ) { ((Engine *) (_pCore->pEngine))->DeleteObject( _pCore, _obj ); } \
	} TRACK_RELEASE

#endif

#define SAFE_KEEP( _obj )       if ( _obj != nullptr ) { _obj->refCount += 1; } TRACK_KEEP

#define OBJECTS_DIFFERENT( OLDOBJ, NEWOBJ ) (OLDOBJ != NEWOBJ)
#define OBJECTS_SAME( OLDOBJ, NEWOBJ ) (OLDOBJ == NEWOBJ)

#define CLEAR_OBJECT( _obj )             (_obj) = nullptr

#define OBJECT_ASSIGN( _pCore, _dstObj, _srcObj ) \
    if ( (_dstObj) != (_srcObj) ) { SAFE_KEEP( (_srcObj) ); SAFE_RELEASE( (_pCore), (_dstObj) ); _dstObj = _srcObj; }

#define GET_SHOW_CONTEXT ShowContext* pShowContext = static_cast<Fiber*>(pCore->pFiber)->GetShowContext();

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
extern int32_t gStatNews;
extern int32_t gStatDeletes;
extern int32_t gStatLinkNews;
extern int32_t gStatLinkDeletes;
extern int32_t gStatIterNews;
extern int32_t gStatIterDeletes;
extern int32_t gStatKeeps;
extern int32_t gStatReleases;

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

#define ALLOCATE_OBJECT( _type, _ptr, _vocab )   _type* _ptr = (_type *) ALLOCATE_BYTES( _vocab->GetSize() );  TRACK_NEW
#define FREE_OBJECT( _obj )  DEALLOCATE_OBJECT( _obj );  TRACK_DELETE
#define ALLOCATE_LINK( _type, _ptr )  _type* _ptr = (_type *) ALLOCATE_BYTES(sizeof(oListElement));  TRACK_LINK_NEW
#define FREE_LINK( _link )  DEALLOCATE_BYTES( _link, sizeof(oListElement) );  TRACK_LINK_DELETE
#define ALLOCATE_ITER( _type, _ptr, _vocab )  ALLOCATE_OBJECT( _type, _ptr, _vocab );  TRACK_ITER_NEW

// UNDELETABLE_OBJECT_REFCOUNT is used for objects like the system object or vocabularies which
//   you don't want to be mistakenly deleted due to refcount mistakes
#define UNDELETABLE_OBJECT_REFCOUNT 2000000000
