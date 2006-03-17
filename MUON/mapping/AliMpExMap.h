/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

// $Id$
// $MpId: AliMpExMap.h,v 1.2 2006/03/17 11:34:29 ivana Exp $

/// \ingroup basic
/// \class AliMpExMap
/// \brief Helper class making Root persistent TExMap
///
/// The objects and keys from TExMap are store in additional
/// arrays which are Root persistent.
///
/// Author: Ivana Hrivnacova; IPN Orsay

#ifndef ALI_MP_EX_MAP_H
#define ALI_MP_EX_MAP_H

#include <TObject.h>
#include <TObjArray.h>
#include <TArrayL.h>
#include <TExMap.h>

class AliMpIntPair;

class TString;

class AliMpExMap : public TObject
{
  public:
    AliMpExMap(Bool_t standardConstructor);
    AliMpExMap();
    virtual ~AliMpExMap();
    
    // static methods
    // conversion between varius keys and Long_t
    //
    static Long_t  GetIndex(const AliMpIntPair& pair);
    static Long_t  GetIndex(const TString& s);
    static AliMpIntPair  GetPair(Long_t index);
    static TString       GetString(Long_t index);

    // set methods
    void Add(const AliMpIntPair& key, TObject* object);
    void Add(const TString& key, TObject* object);
    void Add(Int_t key, TObject* object);

    void SetSize(Int_t size);
    void SetOwner(Bool_t owner);
    
    // get methods
    Int_t       GetSize() const;
    TExMapIter  GetIterator() const;

    TObject*    GetValue(const AliMpIntPair& key) const;
    TObject*    GetValue(const TString& key) const;
    TObject*    GetValue(Int_t key) const;
    
  private:  
    // methods
    void FillMap();
    void AddKey(Long_t key);
    
    // static data members
    static const Int_t    fgkDefaultSize;      // Default initial size
    static const Bool_t   fgkDefaultOwnership; // Default ownership

    static const Int_t    fgkSeparator1; // the separator used for conversion
                                         // of AliMpIntPair to Int_t
    static const Int_t    fgkSeparator2; // the separator used for conversion
                                         // of TString to Int_t
    static const TString  fgkCharacterMap; // the string mapping characters 
                                           // to integers 
    
    // data members
    mutable TExMap  fMap;     //! Transient map class
    TObjArray       fObjects; // Array of objects 
    TArrayL         fKeys;    // Array of keys 

  ClassDef(AliMpExMap,1)  // Root persistent TExMap
};

#endif //ALI_MP_EX_MAP_H

