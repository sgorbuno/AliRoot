/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/* $Id$ */

//_________________________________________________________________________
// Implementation version 1 of algorithm class to construct PHOS track segments
// Track segment for PHOS is list of 
//        EMC RecPoint + (possibly) CPV RecPoint + (possibly) PPSD RecPoint
// To find TrackSegments we do the following: 
//  for each EMC RecPoints we look at
//   CPV/PPSD RecPoints in the radious fRcpv. 
//  If there is such a CPV RecPoint, 
//   we make "Link" it is just indexes of EMC and CPV/PPSD RecPoint and distance
//   between them in the PHOS plane. 
//  Then we sort "Links" and starting from the 
//   least "Link" pointing to the unassined EMC and CPV RecPoints assing them to 
//   new TrackSegment. 
// If there is no CPV/PPSD RecPoint we make TrackSegment 
// consisting from EMC alone. There is no TrackSegments without EMC RecPoint.
//
// In principle this class should be called from AliPHOSReconstructioner, but 
// one can use it as well in standalone mode.
// Use  case:
//  root [0] AliPHOSTrackSegmentMakerv1 * t = new AliPHOSTrackSegmentMaker("galice.root", "tracksegmentsname", "recpointsname")
//  Warning in <TDatabasePDG::TDatabasePDG>: object already instantiated
//               // reads gAlice from header file "galice.root", uses recpoints stored in the branch names "recpointsname" (default = "Default")
//               // and saves recpoints in branch named "tracksegmentsname" (default = "recpointsname")                       
//  root [1] t->ExecuteTask()
//  root [2] t->SetMaxEmcPpsdDistance(5)
//  root [3] t->SetTrackSegmentsBranch("max distance 5 cm")
//  root [4] t->ExecuteTask("deb all time") 
//                 
//*-- Author: Dmitri Peressounko (RRC Ki & SUBATECH) & Yves Schutz (SUBATECH) 
//

// --- ROOT system ---
#include "TROOT.h"
#include "TFile.h"
#include "TFolder.h"
#include "TTree.h"
#include "TSystem.h"
#include "TBenchmark.h"
// --- Standard library ---

#include <iostream.h>
#include <iomanip.h>

// --- AliRoot header files ---

#include "AliPHOSTrackSegmentMakerv1.h"
#include "AliPHOSClusterizerv1.h"
#include "AliPHOSTrackSegment.h"
#include "AliPHOSCpvRecPoint.h"
#include "AliPHOSLink.h"
#include "AliPHOSLoader.h"
#include "AliPHOS.h"
#include "AliRun.h"

ClassImp( AliPHOSTrackSegmentMakerv1) 


//____________________________________________________________________________
  AliPHOSTrackSegmentMakerv1::AliPHOSTrackSegmentMakerv1() : AliPHOSTrackSegmentMaker()
{
  // default ctor (to be used mainly by Streamer)

  InitParameters() ; 

  fTrackSegmentsInRun       = 0 ; 

  fDefaultInit = kTRUE ; 
}

//____________________________________________________________________________
 AliPHOSTrackSegmentMakerv1::AliPHOSTrackSegmentMakerv1(const char * evFoldName, const char * name) : AliPHOSTrackSegmentMaker(evFoldName, name)
{
  // ctor

  InitParameters() ; 
  fTrackSegmentsInRun       = 0 ; 
  Init() ;
  fDefaultInit = kFALSE ; 

}

//____________________________________________________________________________
 AliPHOSTrackSegmentMakerv1::~AliPHOSTrackSegmentMakerv1()
{ 
  // dtor
  // fDefaultInit = kTRUE if TrackSegmentMaker created by default ctor (to get just the parameters)
  
  if (!fDefaultInit) 
   {
    delete fLinkUpArray  ;
   }
}


//____________________________________________________________________________
const TString AliPHOSTrackSegmentMakerv1::BranchName() const 
{  
  TString branchName(GetName() ) ;
//  branchName.Remove(branchName.Index(Version())-1) ;
  return branchName ;
}

//____________________________________________________________________________
void  AliPHOSTrackSegmentMakerv1::FillOneModule()
{
  // Finds first and last indexes between which 
  // clusters from one PHOS module are
  
  AliRunLoader* runget = AliRunLoader::GetRunLoader(GetTitle());
  if(runget == 0x0) 
   {
     Error("FillOneModule","Can not find run getter in event folder \"%s\"",GetTitle());
     return;
   }
  
  AliPHOSLoader* gime = dynamic_cast<AliPHOSLoader*>(runget->GetLoader("PHOSLoader"));
  if ( gime == 0 ) 
   {
     Error("FillOneModule","Could not obtain the Loader object !"); 
     return ;
   } 

   
  TObjArray * emcRecPoints = gime->EmcRecPoints() ; 
  TObjArray * cpvRecPoints = gime->CpvRecPoints() ; 
 
  //First EMC clusters
  Int_t totalEmc = emcRecPoints->GetEntriesFast() ;
  for(fEmcFirst = fEmcLast; (fEmcLast < totalEmc) &&  
       ((dynamic_cast<AliPHOSRecPoint *>(emcRecPoints->At(fEmcLast)))->GetPHOSMod() == fModule ); 
      fEmcLast ++)  ;
  
  //Now CPV clusters
  Int_t totalCpv = cpvRecPoints->GetEntriesFast() ;

    for(fCpvFirst = fCpvLast; (fCpvLast < totalCpv) && 
         ((dynamic_cast<AliPHOSRecPoint *>(cpvRecPoints->At(fCpvLast)))->GetPHOSMod() == fModule ); 
       fCpvLast ++) ;
      
}

//____________________________________________________________________________
Float_t  AliPHOSTrackSegmentMakerv1::GetDistanceInPHOSPlane(AliPHOSEmcRecPoint * emcClu,AliPHOSRecPoint * cpvClu, Bool_t &toofar)const
{
  // Calculates the distance between the EMC RecPoint and the PPSD RecPoint
  // Clusters are sorted in "rows" and "columns" of width 1 cm

  Float_t delta = 1 ;  // Width of the rows in sorting of RecPoints (in cm)
                       // if you change this value, change it as well in xxxRecPoint::Compare()
  Float_t r = fRcpv ;
 
  TVector3 vecEmc ;
  TVector3 vecCpv ;
  
  emcClu->GetLocalPosition(vecEmc) ;
  cpvClu->GetLocalPosition(vecCpv)  ; 

  if(emcClu->GetPHOSMod() == cpvClu->GetPHOSMod()){ 
    if(vecCpv.X() <= vecEmc.X() + fRcpv + 2*delta ){ 

      vecCpv = vecCpv  - vecEmc ; 
      r = vecCpv.Mag() ;
      toofar = kFALSE ;

    } // if  xPpsd >= xEmc + ...
    else 
      toofar = kTRUE ;
  } 
  else 
    toofar = kTRUE ;

  //toofar = kFALSE ;
 
  
  return r ;
}

//____________________________________________________________________________
void  AliPHOSTrackSegmentMakerv1::Init()
{
  // Make all memory allocations that are not possible in default constructor
  
  if ( strcmp(GetTitle(), "") == 0 )
          SetTitle(AliConfig::fgkDefaultEventFolderName) ;
    
  AliRunLoader* runget = AliRunLoader::GetRunLoader(GetTitle());
  if(runget == 0x0) 
   {
     Error("Init","Can not find run getter in event folder \"%s\"",GetTitle());
     return;
   }
  
  AliPHOSLoader* gime = dynamic_cast<AliPHOSLoader*>(runget->GetLoader("PHOSLoader"));
  if ( gime == 0 ) 
   {
     Error("Init","Could not obtain the Loader object !"); 
     return ;
   } 
  
  fLinkUpArray  = new TClonesArray("AliPHOSLink", 1000); 
  gime->PostTrackSegmentMaker(this);
  // create a folder on the white board //YSAlice/WhiteBoard/RecPoints/PHOS/trackSegmentsName
}

//____________________________________________________________________________
void  AliPHOSTrackSegmentMakerv1::InitParameters()
{
  fRcpv      = 10. ;   
  fEmcFirst  = 0 ;    
  fEmcLast   = 0 ;   
  fCpvFirst  = 0 ;   
  fCpvLast   = 0 ;   
  fLinkUpArray = 0 ;
}


//____________________________________________________________________________
void  AliPHOSTrackSegmentMakerv1::MakeLinks()const
{ 
  // Finds distances (links) between all EMC and PPSD clusters, 
  // which are not further apart from each other than fRcpv 
  // and sort them in accordance with this distance
  

  AliRunLoader* runget = AliRunLoader::GetRunLoader(GetTitle());
  if(runget == 0x0) 
   {
     Error("Init","Can not find run getter in event folder \"%s\"",GetTitle());
     return;
   }
  
  AliPHOSLoader* gime = dynamic_cast<AliPHOSLoader*>(runget->GetLoader("PHOSLoader"));
  if ( gime == 0 ) 
   {
     Error("Init","Could not obtain the Loader object !"); 
     return ;
   } 

  TObjArray * emcRecPoints = gime->EmcRecPoints() ; 
  TObjArray * cpvRecPoints = gime->CpvRecPoints() ; 

  fLinkUpArray->Clear() ;    

  AliPHOSRecPoint * cpv ;
  AliPHOSEmcRecPoint * emcclu ;

  Int_t iLinkUp  = 0 ;
  
  Int_t iEmcRP;
  for(iEmcRP = fEmcFirst; iEmcRP < fEmcLast; iEmcRP++ ) {
    emcclu = dynamic_cast<AliPHOSEmcRecPoint *>(emcRecPoints->At(iEmcRP)) ;

    Bool_t toofar ;        
    Int_t iCpv = 0 ;    
    for(iCpv = fCpvFirst; iCpv < fCpvLast;iCpv++ ) { 
      
      cpv = dynamic_cast<AliPHOSRecPoint *>(cpvRecPoints->At(iCpv)) ;
      Float_t r = GetDistanceInPHOSPlane(emcclu, cpv, toofar) ;
      
      if(toofar)
        break ;	 
      if(r < fRcpv) { 
        new ((*fLinkUpArray)[iLinkUp++])  AliPHOSLink(r, iEmcRP, iCpv) ;
      }      
    }
  } 
  
  fLinkUpArray->Sort() ;  //first links with smallest distances
}

//____________________________________________________________________________
void  AliPHOSTrackSegmentMakerv1::MakePairs()
{ 
  // Using the previously made list of "links", we found the smallest link - i.e. 
  // link with the least distance between EMC and CPV and pointing to still 
  // unassigned RecParticles. We assign these RecPoints to TrackSegment and 
  // remove them from the list of "unassigned". 
  

  AliRunLoader* runget = AliRunLoader::GetRunLoader(GetTitle());
  if(runget == 0x0) 
   {
     Error("Init","Can not find run getter in event folder \"%s\"",GetTitle());
     return;
   }
  
  AliPHOSLoader* gime = dynamic_cast<AliPHOSLoader*>(runget->GetLoader("PHOSLoader"));
  if ( gime == 0 ) 
   {
     Error("Init","Could not obtain the Loader object !"); 
     return ;
   } 

  TObjArray * emcRecPoints = gime->EmcRecPoints() ; 
  TObjArray * cpvRecPoints = gime->CpvRecPoints() ; 
  TClonesArray * trackSegments = gime->TrackSegments();
    
  //Make arrays to mark clusters already chosen
  Int_t * emcExist = 0;
  if(fEmcLast > fEmcFirst)
    emcExist = new Int_t[fEmcLast-fEmcFirst] ;
  
  Int_t index;
  for(index = 0; index <fEmcLast-fEmcFirst; index ++)
    emcExist[index] = 1 ;
  
  Bool_t * cpvExist = 0;
  if(fCpvLast > fCpvFirst)
    cpvExist = new Bool_t[fCpvLast-fCpvFirst] ;
  for(index = 0; index <fCpvLast-fCpvFirst; index ++)
    cpvExist[index] = kTRUE ;
  
  
  // Finds the smallest links and makes pairs of CPV and EMC clusters with smallest distance 
  TIter nextUp(fLinkUpArray) ;
  
  AliPHOSLink * linkUp ;
  
  AliPHOSRecPoint * nullpointer = 0 ;
  
  while ( (linkUp =  static_cast<AliPHOSLink *>(nextUp()) ) ){  

    if(emcExist[linkUp->GetEmc()-fEmcFirst] != -1){ //without ppsd Up yet 

      if(cpvExist[linkUp->GetPpsd()-fCpvFirst]){ //CPV still exist
       
       new ((* trackSegments)[fNTrackSegments]) 
         AliPHOSTrackSegment(dynamic_cast<AliPHOSEmcRecPoint *>(emcRecPoints->At(linkUp->GetEmc())) , 
                           dynamic_cast<AliPHOSRecPoint *>(cpvRecPoints->At(linkUp->GetPpsd()))) ;
       (dynamic_cast<AliPHOSTrackSegment *>(trackSegments->At(fNTrackSegments)))->SetIndexInList(fNTrackSegments);
       fNTrackSegments++ ;
       
       emcExist[linkUp->GetEmc()-fEmcFirst] = -1 ; //Mark emc  that Cpv was found 
       //mark CPV recpoint as already used 
       cpvExist[linkUp->GetPpsd()-fCpvFirst] = kFALSE ;
      } //if ppsdUp still exist
    } 
  }        

  //look through emc recPoints left without CPV/PPSD
  if(emcExist){ //if there is emc rec point
    Int_t iEmcRP ;
    for(iEmcRP = 0; iEmcRP < fEmcLast-fEmcFirst  ; iEmcRP++ ){
      if(emcExist[iEmcRP] > 0 ){
       new ((*trackSegments)[fNTrackSegments])  
         AliPHOSTrackSegment(dynamic_cast<AliPHOSEmcRecPoint *>(emcRecPoints->At(iEmcRP+fEmcFirst)), 
                           nullpointer) ;
       (dynamic_cast<AliPHOSTrackSegment *>(trackSegments->At(fNTrackSegments)))->SetIndexInList(fNTrackSegments);
       fNTrackSegments++;    
      } 
    }
  }
  delete [] emcExist ; 
  delete [] cpvExist ; 
}

//____________________________________________________________________________
void  AliPHOSTrackSegmentMakerv1::Exec(Option_t * option)
{
  // STEERing method
  cout<<"\n\n\n  PHOS TRACKING  \n\n";
  if( strcmp(GetName(), "")== 0 ) 
    Init() ;

  if(strstr(option,"tim"))
    gBenchmark->Start("PHOSTSMaker");
 
  if(strstr(option,"print")) {
    Print("") ; 
    return ; 
  }

  AliRunLoader* runget = AliRunLoader::GetRunLoader(GetTitle());
  if(runget == 0x0) 
   {
     Error("Init","Can not find run getter in event folder \"%s\"",GetTitle());
     return;
   }
  
  AliPHOSLoader* gime = dynamic_cast<AliPHOSLoader*>(runget->GetLoader("PHOSLoader"));
  if ( gime == 0 ) 
   {
     Error("Init","Could not obtain the Loader object !"); 
     return ;
   } 
  runget->GetEvent(0);
  if (runget->TreeE() == 0x0) runget->LoadHeader();
  if (gime->TreeR() == 0x0) gime->LoadRecPoints("READ");
  //this causes some warnings because normally
  //loader tries to post if the option is update
  //on the other hand we don't want to recreate because we destroy automatically an old file
  if (gime->TreeT() == 0x0) 
   {
     gime->LoadTracks("UPDATE");
     if (gime->TreeT() == 0x0) gime->MakeTree("T");
   }
  
  
  if( gime->BranchExists("TrackSegments"))
    return ;

  const AliPHOSGeometry * geom = gime->PHOSGeometry() ; 

  Int_t nevents = (Int_t) runget->GetNumberOfEvents();

  Int_t ievent ;

  for(ievent = 0; ievent < nevents; ievent++)
   {
    
    runget->GetEvent(ievent);
    //Make some initializations 
    fNTrackSegments = 0 ;
    fEmcFirst = 0 ;    
    fEmcLast  = 0 ;   
    fCpvFirst = 0 ;   
    fCpvLast  = 0 ;   
    
    gime->TrackSegments()->Clear();

    //    if(!ReadRecPoints(ievent))   continue; //reads RecPoints for event ievent
    
    for(fModule = 1; fModule <= geom->GetNModules() ; fModule++ )
     {
      
      FillOneModule() ; 
      MakeLinks() ;
      MakePairs() ;
      
     }

    WriteTrackSegments(ievent) ;

    if(strstr(option,"deb"))
      PrintTrackSegments(option);
    
    //increment the total number of track segments per run 
    fTrackSegmentsInRun += gime->TrackSegments()->GetEntriesFast() ; 

  }

  if(strstr(option,"tim")){
    gBenchmark->Stop("PHOSTSMaker");
    cout << "AliPHOSTSMaker:" << endl ;
    cout << "  took " << gBenchmark->GetCpuTime("PHOSTSMaker") << " seconds for making TS " 
        <<  gBenchmark->GetCpuTime("PHOSTSMaker")/nevents << " seconds per event " << endl;
    cout << endl ;
  }
  gime->UnloadTracks();
  gime->UnloadRecPoints();
  cout<<"\n  PHOS TRACKING  FINISHED \n\n\n";
    
}

//____________________________________________________________________________
void AliPHOSTrackSegmentMakerv1::Print(Option_t * option)const
{
  //  Print TrackSegmentMaker parameters

  if( strcmp(GetName(), "") != 0 ) {
    cout <<  "======== AliPHOSTrackSegmentMakerv1 ========" << endl ;
    cout <<  "Making Track segments "<< endl ;
    cout <<  "with parameters: " << endl ;
    cout <<  "    Maximal EMC - CPV (PPSD) distance (cm)" << fRcpv << endl ;
    cout <<  "============================================" << endl ;
  }
  else
    cout << "AliPHOSTrackSegmentMakerv1 not initialized " << endl ;
}

//____________________________________________________________________________
void AliPHOSTrackSegmentMakerv1::WriteTrackSegments(Int_t event)
{
  // Writes found TrackSegments to TreeR. Creates branches 
  // "PHOSTS" and "AliPHOSTrackSegmentMaker" with the same title.
  // In the former branch found TrackSegments are stored, while 
  // in the latter all parameters, with which TS were made. 
  // ROOT does not allow overwriting existing branches, therefore
  // first we check, if branches with the same title already exist.
  // If yes - exits without writing.
  
  AliPHOSLoader* gime = AliPHOSLoader::GetPHOSLoader(GetTitle());
  if ( gime == 0 ) 
   {
     Error("Init","Could not obtain the Loader object !"); 
     return ;
   } 

  TClonesArray * trackSegments = gime->TrackSegments() ; 
  trackSegments->Expand(trackSegments->GetEntriesFast()) ;

  TTree * treeT = gime->TreeT();
  if(!treeT){
    gime->MakeTree("T");
    treeT = gime->TreeT() ;
  }

  //First TS
  Int_t bufferSize = 32000 ;    

  TBranch * tsBranch = treeT->Branch(gime->GetDetectorName(),&trackSegments,bufferSize);
  tsBranch->Fill() ;  

  gime->WriteTracker("OVERWRITE");
  gime->WriteTracks("OVERWRITE");
}


//____________________________________________________________________________
void AliPHOSTrackSegmentMakerv1::PrintTrackSegments(Option_t * option)
{
  // option deb - prints # of found TrackSegments
  // option deb all - prints as well indexed of found RecParticles assigned to the TS

  AliRunLoader* runget = AliRunLoader::GetRunLoader(GetTitle());
  if(runget == 0x0) 
   {
     Error("Init","Can not find run getter in event folder \"%s\"",GetTitle());
     return;
   }

  AliPHOSLoader* gime = AliPHOSLoader::GetPHOSLoader(GetTitle());
  if ( gime == 0 ) 
   {
     Error("Init","Could not obtain the Loader object !"); 
     return ;
   } 

  TClonesArray * trackSegments = gime->TrackSegments() ; 
  if (trackSegments == 0x0)
   {//check if pointer exists - 
     gime->LoadTracks();
     trackSegments = gime->TrackSegments(); 
     if (trackSegments == 0x0)
      {
       Error("PrintTrackSegments","Can not get track segments for current event = %d",
             runget->GetEventNumber());
       return;
      }
   }
  
  cout << "AliPHOSTrackSegmentMakerv1: event "<<runget->GetEventNumber()  << endl ;
  cout << "       Found " << trackSegments->GetEntriesFast() << "  trackSegments " << endl ;
  
  if(strstr(option,"all")) {  // printing found TS
    cout << "TrackSegment # " << "    EMC RP#    " << "    CPV RP#    " << endl ; 
    
    Int_t index;
    for (index = 0 ; index <trackSegments->GetEntriesFast() ; index++) {
      AliPHOSTrackSegment * ts = (AliPHOSTrackSegment * )trackSegments->At(index) ; 
      cout<<"   "<< setw(4) << ts->GetIndexInList() << "            " 
         <<setw(4) << ts->GetEmcIndex()<< "            " 
         <<setw(4) << ts->GetCpvIndex()<< "            " << endl ;
    }       
    
    cout << "-------------------------------------------------------"<< endl ;
  }
}
