#ifndef ALIMUONSEGMENTATIONTRIGGERX_H
#define ALIMUONSEGMENTATIONTRIGGERX_H

/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */

#include "AliMUONSegmentationTrigger.h"

class AliMUONChamber;
//----------------------------------------------
//
// Chamber segmentation virtual base class
//
class AliMUONSegmentationTriggerX :
public AliMUONSegmentationTrigger {
 public:
  AliMUONSegmentationTriggerX(){}
  virtual ~AliMUONSegmentationTriggerX(){}
  // Transform from pad to real coordinates
  virtual void    GetPadIxy(Float_t x, Float_t y, Int_t &ix, Int_t &iy);
  virtual void    GetPadIxy(Float_t x, Float_t y, Float_t z, Int_t &ix, Int_t &iy)
      {GetPadIxy(x, y, ix, iy);}
  // Transform from real to pad coordinates
  virtual void    GetPadCxy(Int_t ix, Int_t iy, Float_t &x, Float_t &y);
  virtual void    GetPadCxy(Int_t ix, Int_t iy, Float_t &x, Float_t &y, Float_t &z)
      {z=0; GetPadCxy(ix, iy, x , y);}
  // Pad size Dx*Dy 
  virtual void SetPadSize(Float_t dp1, Float_t dp2);
  // Strip size 
  virtual Float_t Dpx(Int_t imodule);
  virtual Float_t Dpy(Int_t imodule);
  // Set pad position
  virtual void     SetPad(Int_t ix, Int_t iy);
  // Set hit position
  virtual void     SetHit(Float_t xhit, Float_t yhit);
  virtual void     SetHit(Float_t xhit, Float_t yhit, Float_t zhit)
      {SetHit(xhit, yhit);}
  // Current integration parameters
  virtual void IntegrationLimits(Float_t& x1, Float_t& x2, Float_t& x3, Float_t& width);
  // Current Pad during Integration
  // x-coordinate
  virtual Int_t  Ix();
  // y-coordinate
  virtual Int_t  Iy();
  // Sector
  virtual Int_t ISector();
  // calculate sector from pad coordinates
  virtual Int_t Sector(Int_t ix, Int_t iy);
  // Get next neighbours 
  virtual void Neighbours
    (Int_t iX, Int_t iY, Int_t* Nlist, Int_t Xlist[2], Int_t Ylist[2]);
  
  //
  // Initialisation
  virtual void Init(AliMUONChamber* chamber);    
  
  ClassDef(AliMUONSegmentationTriggerX,1) //Segmentation class for trigger X
    
  protected:
   void  IntegrationParam(Float_t& x1, Float_t& x2, Float_t& y1);
// Geometry Parameters
  float fXofxsmin[126][64];    // x-min
  float fXofxsmax[126][64];    // x-max
  float fYofxsmin[126][64];    // y-min
  float fYofxsmax[126][64];    // y-max

// Current pad during integration (cursor for disintegration)
  Int_t fix;  // pad coord.  x 
  Int_t fiy;  // pad coord.  y 
  Float_t fx; // real coord. x
  Float_t fy; // real ccord. y

};
#endif



