#ifndef ALICRT_H
#define ALICRT_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */

////////////////////////////////////////////////
//  Manager class for detector: ACORDE        //
////////////////////////////////////////////////

#include "AliDetector.h"

class AliCRT : public AliDetector {
public:
  AliCRT();
  AliCRT(const char* name, const char* title);
  AliCRT(const AliCRT& crt);
  virtual ~AliCRT();

  AliCRT& operator=(const AliCRT& crt);
  virtual void CreateMaterials();

  virtual Int_t IsVersion() const { return 0; }

  virtual TString Version() { return TString(""); }

  virtual void SetTreeAddress();

private:
  ClassDef(AliCRT, 1) // Cosmic Ray Trigger (ACORDE) base class
};


// inline TString AliCRT::Version()
#endif // ALICRT_H
