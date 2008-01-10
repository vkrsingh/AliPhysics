#ifndef ALICFMANAGER_H
#define ALICFMANAGER_H
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
// Prototype class helping the user to handle event & particle-level 
// selections/correction containers inside the Analysis job
// Author:S.Arcelli. Silvia.Arcelli@cern.ch 

#include "TNamed.h"

class TObject;
class TList;
class TBits;
class AliCFContainer ;
class AliMCEventHandler ;
class AliGenEventHeader ;
//____________________________________________________________________________
class AliCFManager : public TNamed 
{
 public :
  AliCFManager() ;
  AliCFManager(Char_t* name, Char_t* title) ;
  AliCFManager(const AliCFManager& c) ;
  AliCFManager& operator=(const AliCFManager& c) ;
  virtual ~AliCFManager();

  //
  //Currently foreseen selection steps for event-level cuts:
  //generator, trigger, reconstruction
  //
  enum{
    kEvtGenCuts=0,
      kEvtTrigCuts,
      kEvtRecCuts,
      kNEvtSel=3
      };

  //
  //Currently foreseen selection steps for particle-level cuts:
  //generator, acceptance, reconstruction, user selection
  //

  enum{
      kPartGenCuts=0,
      kPartAccCuts,
      kPartRecCuts,
      kPartSelCuts,
      kNPartSel=4
      };

  //
  // Setters:
  //
  //pass the pointer to the correction container
  virtual void SetEventContainer(AliCFContainer* c) {fEvtContainer=c;} ; 

  //pass the pointer to the correction container
  virtual void SetParticleContainer(AliCFContainer* c) {fPartContainer=c;} ; 

  //Setter for event-level selection cut list at selection step isel
  virtual void SetEventCutsList(Int_t isel, TObjArray* array) {fEvtCutList[isel]=array;} ; 

  //Setter for particle-level selection cut list at selection step isel
  virtual void SetParticleCutsList(Int_t isel, TObjArray* array) {fPartCutList[isel]=array;} ; 


  //
  //Getters
  //
  // pointer to the Event-level correction container
  virtual AliCFContainer* GetEventContainer() const {return fEvtContainer;} ; 

  // pointer to the Particle-level correction container
  virtual AliCFContainer* GetParticleContainer() const {return fPartContainer;} ; 

  //pointer to the event-level cut list for event selection step isel
  virtual TObjArray* GetEventCutsList(Int_t isel) const {return fEvtCutList[isel];};

//pointer to the particle-level cut list for particle selection step isel
  virtual TObjArray* GetParticleCutsList(Int_t isel) const {return fPartCutList[isel];};
  

  //Pass the pointer to obj to the selections (used to access MC/rec global
  //event info when requested
  virtual void SetEventInfo(TObject *obj) const;

  //Cut Checkers: by default *all* the cuts of a given input list is checked 
  //(.and. of all cuts), but the user can select a subsample of cuts in the 
  //list via the string argument selcuts
 
  virtual Bool_t CheckEventCuts(Int_t isel, TObject *obj, const TString &selcuts="all") const;
  virtual Bool_t CheckParticleCuts(Int_t isel, TObject *obj, const TString &selcuts="all") const;

  virtual void FillQABeforeEventCuts(Int_t isel, TObject *obj) const;
  virtual void FillQAAfterEventCuts(Int_t isel, TObject *obj) const;
  virtual void FillQABeforeParticleCuts(Int_t isel, TObject *obj) const;
  virtual void FillQAAfterParticleCuts(Int_t isel, TObject *obj) const;

  virtual void InitQAHistos() const;//init QA histograms
  virtual TBits*  GetQAParticleSelBits(Int_t isel, TObject *obj);//get cut mask
  virtual void AddQAHistosToList(TList *list) const; //put the QA histos in TList
  
  
 private:

  //the correction grid
  AliCFContainer *fEvtContainer; //ptr to Evt-Level correction container
  //the correction grid
  AliCFContainer *fPartContainer; //ptr to Particle-level correction container
  //Evt-Level Selections
  TObjArray *fEvtCutList[kNEvtSel]; //arrays of cuts: gen,trig,rec-level
  //Particle-level selections
  TObjArray *fPartCutList[kNPartSel]; //arrays of cuts: gen,acceptance,rec,sel-level
  TBits *fhQABits; // Global list of Cuts' QA BitMaps

  Bool_t CompareStrings(const TString  &cutname,const TString  &selcuts) const;

  ClassDef(AliCFManager,1);
};


#endif
