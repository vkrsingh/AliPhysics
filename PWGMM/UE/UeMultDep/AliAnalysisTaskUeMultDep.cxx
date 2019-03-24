/*   This macro produces: Delta phi correclation in different multiplicity classes and leading particle dE/dx vs leading particle momentum vs Multiplicity (near side, away side and transverse side) 
   Aditya Nath Mishra ICN-UNAM
   Please report bugs to: amishra@cern.ch / aditya.mishra@correo.nucleares.unam.mx 
   First version: 07/03/2019

   Calibration factors of dE/dx were provided by Omar Vazquez (Lund University)

 */

#include "AliAnalysisTaskUeMultDep.h"

// ROOT includes
#include <TList.h>
#include <TMath.h>
#include <TH1.h>
#include <TH2D.h>
#include <TProfile.h>
#include <THnSparse.h>
#include <TFile.h>


// AliRoot includes
#include <AliAnalysisManager.h>
#include <AliAnalysisFilter.h>
#include <AliESDInputHandler.h>
#include <AliESDEvent.h>
#include <AliEventCuts.h>
#include <AliESDVertex.h>
#include <AliLog.h>
#include <AliExternalTrackParam.h>
#include <AliESDtrackCuts.h>
#include <AliESDVZERO.h>
#include <AliAODVZERO.h>
#include <TTreeStream.h>
#include <AliHeader.h>
#include <AliAnalysisUtils.h>
#include <AliMultiplicity.h>
#include <AliMultSelection.h>
#include <AliMultSelectionTask.h>
#include <AliAODInputHandler.h> 
#include <AliAODHandler.h> 
#include <AliAODVertex.h>
#include <AliAODTrack.h> 
#include <AliAODPid.h> 
#include <AliDataFile.h>

#include <iostream>
using namespace std;

const Char_t * legpTL[10] = {"0.15<#it{p}^{leading}<200", "1.5<#it{p}^{leading}<2.0", "4.0<#it{p}^{leading}<4.5", "7.0<#it{p}^{leading}<8.0", "10<#it{p}^{leading}<20", "20<#it{p}^{leading}<30", "30<#it{p}^{leading}<50", "50<#it{p}^{leading}<70", "70<#it{p}^{leading}<100", "100<#it{p}^{leading}<300"};

const Double_t pi = 3.1415926535897932384626433832795028841971693993751058209749445;

ClassImp(AliAnalysisTaskUeMultDep)

	//_____________________________________________________________________________
	AliAnalysisTaskUeMultDep::AliAnalysisTaskUeMultDep():
		AliAnalysisTaskSE(),
		fESD(0x0),
       		fEventCuts(0x0),
		fTrackFilter(0x0),
		fAnalysisType("ESD"),
                fNcl(70),
		fListOfObjects(0x0),
		fHistEventCounter(0x0),
                hPhi(0x0),
                hpT(0x0),
                hPtL(0x0),
         	hPhiL(0x0),
        	hEtaL(0x0),
		hDphi(0x0),
                hDphiNS(0x0),
		hDphiAS(0x0),
	        hDphiTS(0x0),
	        hRefMult08(0x0),
		hV0Mmult(0x0),
		hpTvsNch(0x0),
		hpTLvsNch(0x0),
                hpTLvsNchNS(0x0),
        	hpTLvsNchAS(0x0),
         	hpTLvsNchTS(0x0),
        	ProfpTLvsNchNS(0x0),
        	ProfpTLvsNchAS(0x0),
          	ProfpTLvsNchTS(0x0),  
		hpTvsRefMult08(0x0),
		hpTLvsRefMult08(0x0),
		hpTvsV0Mmult(0x0),
		hpTLvsV0Mmult(0x0),
		hRefMultvsV0Mmult(0x0),
		hpTvsV0MmultvsRefMult08(0x0),
		hpTLvsV0MmultvsRefMult08(0x0),
                hpTLvsRefMult08vsDphi(0x0),
		hpTLvsV0MmultvsDphi(0x0),
                hptLvsv0MvsRefMultvsDphivsDeta(0x0),
                fMultSelection(0x0),
                ftrackmult08(-999),   
                fv0mpercentile(-999)

{
	// Default constructor (should not be used)
}

//______________________________________________________________________________
AliAnalysisTaskUeMultDep::AliAnalysisTaskUeMultDep(const char *name):
	AliAnalysisTaskSE(name),
	fESD(0x0),
	fEventCuts(0x0),
	fTrackFilter(0x0),
	fAnalysisType("ESD"),
	fNcl(70),
	fListOfObjects(0x0),
	fHistEventCounter(0x0), 
	hPhi(0x0),
	hpT(0x0),
	hPtL(0x0),
	hPhiL(0x0),
	hEtaL(0x0),
	hDphi(0x0),
	hDphiNS(0x0),
	hDphiAS(0x0),
	hDphiTS(0x0),
	hRefMult08(0x0),
	hV0Mmult(0x0),
	hpTvsNch(0x0),	
	hpTLvsNch(0x0),
	hpTLvsNchNS(0x0),
	hpTLvsNchAS(0x0),
	hpTLvsNchTS(0x0),
	ProfpTLvsNchNS(0x0),
	ProfpTLvsNchAS(0x0),
	ProfpTLvsNchTS(0x0),
	hpTvsRefMult08(0x0),
	hpTLvsRefMult08(0x0),
	hpTvsV0Mmult(0x0),
	hpTLvsV0Mmult(0x0),
	hRefMultvsV0Mmult(0x0),
	hpTvsV0MmultvsRefMult08(0x0),
	hpTLvsV0MmultvsRefMult08(0x0),
	hpTLvsRefMult08vsDphi(0x0),
	hpTLvsV0MmultvsDphi(0x0),
	hptLvsv0MvsRefMultvsDphivsDeta(0x0),
	fMultSelection(0x0),
	ftrackmult08(-999),   
	fv0mpercentile(-999)
{
	DefineOutput(1, TList::Class());
}

//________________________________________________________________________

void AliAnalysisTaskUeMultDep::Exit(const char *msg) {

	Printf("%s", msg);
	return;
}


//_____________________________________________________________________________
AliAnalysisTaskUeMultDep::~AliAnalysisTaskUeMultDep()
{
	// Destructor
	// histograms are in the output list and deleted when the output
	// list is deleted by the TSelector dtor
	if (fListOfObjects && !AliAnalysisManager::GetAnalysisManager()->IsProofMode()){
		delete fListOfObjects;
		fListOfObjects = 0x0;
	}

}

//______________________________________________________________________________
void AliAnalysisTaskUeMultDep::UserCreateOutputObjects()
{
	const Int_t nNchBins = 200;
	Double_t NchBins[nNchBins+1]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,
				21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,
				39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,
				57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,
				75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,
				93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,
				109,110,111,112,113,114,115,116,117,118,119,120,121,122,
				123,124,125,126,127,128,129,130,131,132,133,134,135,136,
				137,138,139,140,141,142,143,144,145,146,147,148,149,150,
				151,152,153,154,155,156,157,158,159,160,161,162,163,164,
				165,166,167,168,169,170,171,172,173,174,175,176,177,178,
				179,180,181,182,183,184,185,186,187,188,189,190,191,192,
				193,194,195,196,197,198,199,200};

	const Int_t nPtBins      = 61;
	Double_t xBins[nPtBins+1] = {
                             0.5,0.55,0.6,0.65,0.7,0.75,0.8,0.85,0.9,0.95,1,1.1,1.2,1.3,1.4,1.5,
			     1.6,1.7,1.8,1.9,2,2.2,2.4,2.6,2.8,3,3.2,3.4,3.6,3.8,4,4.5,5,5.5,6,
			     6.5,7,8,9,10,11,12,13,14,15,16,18,20,22.0,24.0,26.0,28.0,32.0,36.0,
			     42.0,50.0,60.0,80.0,100.0,130.0,160.0,200.0};

	const Int_t nV0Mbins = 10;
	Double_t V0Mbins[nV0Mbins+1]={0.00, 1.00, 5.00, 10.0, 15.0, 20.0, 30.0, 40.0, 50.0, 70.0, 100};

	// This method is called once per worker node
	// Here we define the output: histograms and debug tree if requested 

	// Definition of trackcuts
	if(!fTrackFilter){	
		fTrackFilter = new AliAnalysisFilter("trackFilter2015");
		SetTrackCuts(fTrackFilter);
	}

	OpenFile(1);
	fListOfObjects = new TList();
	fListOfObjects->SetOwner();

	//
	// Histograms
	//  
	if(! fHistEventCounter ){
		fHistEventCounter = new TH1D( "fHistEventCounter", ";Evt. Sel. Step;Count",10,0,10);

		fHistEventCounter->GetXaxis()->SetBinLabel(1, "Processed");
		fHistEventCounter->GetXaxis()->SetBinLabel(2, "Trigger");//NotinVertexcut");
		fHistEventCounter->GetXaxis()->SetBinLabel(3, "Physics Selection"); //CINT7-B-NOPF-CENT");
		fListOfObjects->Add(fHistEventCounter);
	}

	hPhi = new TH1D("hPhi", ";#phi (rad); count", 64,0,2.0*TMath::Pi());
	fListOfObjects->Add(hPhi);

	hpT = new TH1D("hpT","",nPtBins,xBins);
	fListOfObjects->Add(hpT);

	hDphi = 0;
	hDphi = new TH1D("hDphi","",2*64,-2*TMath::Pi(),2*TMath::Pi());
	fListOfObjects->Add(hDphi);

	hDphiNS = 0;
	hDphiNS = new TH1D("hDphiSN","",2*64,-2*TMath::Pi(),2*TMath::Pi());
	fListOfObjects->Add(hDphiNS);

	hDphiAS = 0;
	hDphiAS = new TH1D("hDphiAS","",2*64,-2*TMath::Pi(),2*TMath::Pi());
	fListOfObjects->Add(hDphiAS);

	hDphiTS = 0;
	hDphiTS = new TH1D("hDphiTS","",2*64,-2*TMath::Pi(),2*TMath::Pi());
	fListOfObjects->Add(hDphiTS);

	hRefMult08 = 0;
	hRefMult08 = new TH1D("hRefMult08","Multiplicity (-0.8 < #eta < 0.8);N_{ch};count",nNchBins,NchBins);
	fListOfObjects->Add(hRefMult08);
	
	hV0Mmult = 0;
	hV0Mmult = new TH1D("hV0Mmult","V0M ;V0M percentile;count",500,0,500);
	fListOfObjects->Add(hV0Mmult);
	
	hpTvsNch = 0;
	hpTvsNch = new TH2D("hpTvsNch","p_{T} vs N_{ch}; #it{p}_{T} (GeV/#it{c}); N_{ch} (|#eta| < 0.8 & p_{T} > 0.5 GeV/c)",nPtBins,xBins,200,0,200);
	fListOfObjects->Add(hpTvsNch);
	
	hpTLvsNch = 0;
	hpTLvsNch = new TH2D("hpTLvsNch","p_{T}^{leading} vs N_{ch}; #it{p}_{T}^{leading} (GeV/#it{c});N_{ch} (|#eta| < 0.8 & p_{T} > 0.5 GeV/c)",nPtBins,xBins,200,0,200);
	fListOfObjects->Add(hpTLvsNch);
		
	hpTvsRefMult08 = 0;
	hpTvsRefMult08 = new TH2D("hpTvsRefMult08","p_{T} vs RefMult08; #it{p}_{T} (GeV/#it{c}); RefMult08 (|#eta| < 0.8 & p_{T} > 0.5 GeV/c)",nPtBins,xBins,200,0,200);
	fListOfObjects->Add(hpTvsRefMult08);
	
	hpTLvsRefMult08 = 0;
	hpTLvsRefMult08 = new TH2D("hpTLvsRefMult08","p_{T}^{leading} vs RefMult08; #it{p}_{T}^{leading} (GeV/#it{c}); RefMult08 (|#eta| < 0.8 & p_{T} > 0.5 GeV/c)",nPtBins,xBins,200,0,200);
	fListOfObjects->Add(hpTLvsRefMult08);
	
	hpTvsV0Mmult = 0;
	hpTvsV0Mmult = new TH2D("hpTvsV0Mmult","p_{T} vs V0Mmult; #it{p}_{T} (GeV/#it{c}); V0Mmult (|#eta| < 0.8 & p_{T} > 0.5 GeV/c)",nPtBins,xBins,200,0,200);
	fListOfObjects->Add(hpTvsV0Mmult);
	
	hpTLvsV0Mmult = 0;
	hpTLvsV0Mmult = new TH2D("hpTLvsV0Mmult","p_{T}^{leading} vs V0Mmult; #it{p}_{T}^{leading} (GeV/#it{c}); V0Mmult (|#eta| < 0.8 & p_{T} > 0.5 GeV/c)",nPtBins,xBins,200,0,200);
	fListOfObjects->Add(hpTLvsV0Mmult);
	
	hRefMultvsV0Mmult = 0;
	hRefMultvsV0Mmult = new TH2D("hRefMultvsV0Mmult","N_{ch} vs V0M percentile;N_{ch}; v0M percentile",nNchBins,NchBins,200,0,200);
	fListOfObjects->Add(hRefMultvsV0Mmult);

	hpTvsV0MmultvsRefMult08 = 0;
	hpTvsV0MmultvsRefMult08 = new TH3D("hpTvsV0MmultvsRefMult08","p_{T} vs v0M percentile vs RefMult08;#it{p}_{T} (GeV/c);V0M percentile;RefMult08",nPtBins,xBins,nV0Mbins,V0Mbins,nNchBins,NchBins);
	fListOfObjects->Add(hpTvsV0MmultvsRefMult08);

	hpTLvsV0MmultvsRefMult08 = 0;
	hpTLvsV0MmultvsRefMult08 = new TH3D("hpTLvsV0MmultvsRefMult08","p_{T}^{Leading} vs v0M percentile vs RefMult08;#it{p}_{T}^{Leading} (GeV/c);V0M percentile;RefMult08",nPtBins,xBins,nV0Mbins,V0Mbins,nNchBins,NchBins);
	fListOfObjects->Add(hpTLvsV0MmultvsRefMult08);
	
	hpTLvsRefMult08vsDphi = 0;
	hpTLvsRefMult08vsDphi = new TH3D("hpTLvsRefMult08vsDphi","p_{T}^{Leading} vs RefMult08 vs Dphi;#it{p}_{T}^{Leading} (GeV/c);RefMult08; #delta#phi (rad)",200,0,200,200,0,200,64,-(TMath::Pi())/2.0,3.0*(TMath::Pi())/2.0);
	fListOfObjects->Add(hpTLvsRefMult08vsDphi);

	hpTLvsV0MmultvsDphi = 0;
	hpTLvsV0MmultvsDphi = new TH3D("hpTLvsV0MmultvsDphi","p_{T}^{Leading} vs V0Mmult vs Dphi;#it{p}_{T}^{Leading} (GeV/c);V0Mmult; #Delta#phi (rad)",200,0,200,200,0,200,64,-(TMath::Pi())/2.0,3.0*(TMath::Pi())/2.0);
	fListOfObjects->Add(hpTLvsV0MmultvsDphi);

	hPtL = 0;
	hPtL = new TH1D("hPtL",";#it{p}_{T}^{leading} (GeV/#it{c});counts",nPtBins,xBins);
	fListOfObjects->Add(hPtL);
	
	hEtaL = 0;
	hEtaL = new TH1D("hEtaL","; #eta^{leading};counts",20,-1,1);
	fListOfObjects->Add(hEtaL);
	
	hPhiL = 0;
	hPhiL = new TH1D("hPhiL","; #phi^{leading} (rad);counts",64,0,2.0*TMath::Pi());
	fListOfObjects->Add(hPhiL);
	
	ProfpTLvsNchNS = 0;
	ProfpTLvsNchNS = new TProfile("ProfpTLvsNchNS","Near side; #it{p}^{leading} (GeV/#it{c}); #it{N}_{ch} (#it{p}_{T}>0.5 GeV/#it{c})",nPtBins,xBins,0,20);
	fListOfObjects->Add(ProfpTLvsNchNS);

	ProfpTLvsNchAS = 0;
	ProfpTLvsNchAS = new TProfile("ProfpTLvsNchAS","Away side; #it{p}^{leading} (GeV/#it{c}); #it{N}_{ch} (#it{p}_{T}>0.5 GeV/#it{c})",nPtBins,xBins,0,20);
	fListOfObjects->Add(ProfpTLvsNchAS);

	ProfpTLvsNchTS = 0;
	ProfpTLvsNchTS = new TProfile("ProfpTLvsNchTS","Transverse side; #it{p}^{leading} (GeV/#it{c}); #it{N}_{ch} (#it{p}_{T}>0.5 GeV/#it{c})",nPtBins,xBins,0,20);
	fListOfObjects->Add(ProfpTLvsNchTS);
	
	hpTLvsNchNS = 0;
	hpTLvsNchNS = new TH2D("hpTLvsNchNS","Near side; #it{p}_{T}^{leading} (GeV/#it{c});N_{ch} (|#eta| < 0.8 & p_{T} > 0.5 GeV/c)",nPtBins,xBins,200,0,200);
	fListOfObjects->Add(hpTLvsNchNS);

	hpTLvsNchAS = 0;
	hpTLvsNchAS = new TH2D("hpTLvsNchAS","Away side; #it{p}_{T}^{leading} (GeV/#it{c});N_{ch} (|#eta| < 0.8 & p_{T} > 0.5 GeV/c)",nPtBins,xBins,200,0,200);
	fListOfObjects->Add(hpTLvsNchAS);

	hpTLvsNchTS = 0;
	hpTLvsNchTS = new TH2D("hpTLvsNchTS","Transverse side; #it{p}_{T}^{leading} (GeV/#it{c});N_{ch} (|#eta| < 0.8 & p_{T} > 0.5 GeV/c)",nPtBins,xBins,200,0,200);
	fListOfObjects->Add(hpTLvsNchTS);

		
	// defining THnSparseD....
	const Int_t dims = 6;
	
	Int_t bins[dims] = {200, 200, 200, 200, 64, 20};
	Double_t xmin[dims] = {0, 0., 0, 0, -(TMath::Pi())/2.0, -1.0};
	Double_t xmax[dims] = {200, 200., 200, 200, 3.0*(TMath::Pi())/2.0, 1.0};
	
	hptLvsv0MvsRefMultvsDphivsDeta = new THnSparseD("hptLvsv0MvsRefMultvsDphivsDeta","", dims, bins, xmin, xmax);
	fListOfObjects->Add(hptLvsv0MvsRefMultvsDphivsDeta);

	fEventCuts.AddQAplotsToList(fListOfObjects);

	PostData(1, fListOfObjects);

}

//______________________________________________________________________________
void AliAnalysisTaskUeMultDep::UserExec(Option_t *)
{

	// -----------------------------------------------------
	//			 InputEvent
	// -----------------------------------------------------

	AliVEvent *event = InputEvent();
	if (!event) {
		Error("UserExec", "Could not retrieve event");
		return;
	}

	// -----------------------------------------------------
	//			 E S D
	// -----------------------------------------------------

	if (fAnalysisType == "ESD"){
		fESD = dynamic_cast<AliESDEvent*>(event);

		if(!fESD){
			Printf("%s:%d ESDEvent not found in Input Manager",(char*)__FILE__,__LINE__);
			this->Dump();
			return;
		}
	}

	fHistEventCounter->Fill(0.5); 	//	All events

	AliAnalysisUtils * utils = new AliAnalysisUtils();
	if (!utils)
	{
		cout<<"------- No AnalysisUtils Object Found --------"<<utils<<endl;
		return;
	}

	//Bool_t fSPDCvsTCutStatus = kFALSE;

	//fSPDCvsTCutStatus = utils->IsSPDClusterVsTrackletBG(fESD);

	// Cuts at event level
	UInt_t fSelectMask= fInputHandler->IsEventSelected();
	Bool_t isINT7selected = fSelectMask&AliVEvent::kINT7;
	if(!isINT7selected)
		return;
	fHistEventCounter->Fill(1.5);

	if (!fEventCuts.AcceptEvent(event)) {
		PostData(1, fListOfObjects);
		return;
	}

	fHistEventCounter->Fill(2.5);

	// -------------------------------------- multiplcity estimators section ------------------------------------------ //

	ftrackmult08 = -999;
	fv0mpercentile = -999;

	//ftrackmult08=AliESDtrackCuts::GetReferenceMultiplicity(fESD, AliESDtrackCuts::kTrackletsITSTPC, 0.8);     //reference
	ftrackmult08=AliESDtrackCuts::GetReferenceMultiplicity(fESD, AliESDtrackCuts::kTracklets, 0.8);     //tracklets
	//ftrackmult08 = AliPPVsMultUtils::GetStandardReferenceMultiplicity(fESD); //Combined estimator

	hRefMult08->Fill(ftrackmult08);

	fMultSelection = (AliMultSelection*) fESD->FindListObject("MultSelection"); // Esto es para 13 TeV
	if (!fMultSelection)
		cout<<"------- No AliMultSelection Object Found --------"<<fMultSelection<<endl;
	else
	fv0mpercentile = fMultSelection->GetMultiplicityPercentile("V0M");
	hV0Mmult->Fill(fv0mpercentile);

	cout<<"------- V0M mult ==  "<<fv0mpercentile<<"--------"<<endl;
	
	hRefMultvsV0Mmult->Fill(ftrackmult08,fv0mpercentile);

	// ------------------------------------------ end of mult estimators -------------------------------------------------//
	
	MakeAnalysis(0.8);
	cout<<"hello!!!"<<endl;

	// Post output data.
	PostData(1, fListOfObjects);

}
//________________________________________________________________________
void AliAnalysisTaskUeMultDep::MakeAnalysis( Double_t etaCut ){


	// selection on leading particle
	Double_t pt_leading    = 0;
	Double_t p_leading    = 0;
	Double_t eta_leading    = 0;
	Double_t phi_leading    = 0;

	Int_t    i_leading = 0;
	Double_t vals[5];

	for(Int_t i = 0; i < fESD->GetNumberOfTracks(); i++) {

		AliESDtrack* esdTrack = fESD->GetTrack(i);

		Double_t eta      = esdTrack->Eta();
		Double_t phi      = esdTrack->Phi();
		Double_t momentum = esdTrack->P();
		Double_t pt       = esdTrack->Pt();

		hPhi->Fill(phi);
	
		if(TMath::Abs(eta) > etaCut) continue;

		//quality cuts, standard 2015 track cuts
		if(!fTrackFilter->IsSelected(esdTrack)) continue;

		if(pt<0.5) continue;

		Short_t ncl = esdTrack->GetTPCsignalN();
		if(ncl<fNcl) continue;
	
		if(pt>pt_leading){
			pt_leading      = pt;
			p_leading       = momentum;
			eta_leading     = eta;
			phi_leading     = phi;
			i_leading = i;
		}

		hpT->Fill(pt);

	}// end loop over tracks

	if(pt_leading<2.0)
		return;

	hPtL->Fill(pt_leading);
	hEtaL->Fill(eta_leading);
	hPhiL->Fill(phi_leading);

	// Next step: pTL vs Number density (NS, AS, TS)
	Double_t mult_ns = 0;
	Double_t mult_nns = 0;
	Double_t mult_as = 0;
	Double_t mult_ts = 0;
	Double_t mult    = 0;

	for(Int_t i = 0; i < fESD->GetNumberOfTracks(); i++) {

		// exclude the auto-correlation
		if(i==i_leading)
			continue;

		AliESDtrack* esdTrack = fESD->GetTrack(i);

		Double_t eta      = esdTrack->Eta();
		Double_t phi      = esdTrack->Phi();
		Double_t pt       = esdTrack->Pt();

		if(TMath::Abs(eta) > etaCut)
			continue;

		//quality cuts, standard 2015 track cuts
		if(!fTrackFilter->IsSelected(esdTrack))
			continue;

		if(pt<0.5)// only above 500 GeV/c
			continue;
		mult++;
		
		Double_t DPhi = DeltaPhi( phi, phi_leading );
		Double_t DEta = TMath::Abs( eta -  eta_leading );
		//	Double_t R = TMath::Sqrt(DPhi*DPhi+DEta*DEta);
		Double_t DeltaEta = eta -  eta_leading;

		hDphi->Fill(DPhi);
		hpTvsNch->Fill(pt,mult);
		hpTvsRefMult08->Fill(pt,ftrackmult08);
		hpTvsV0Mmult->Fill(pt,fv0mpercentile);
		hpTvsV0MmultvsRefMult08->Fill(pt,fv0mpercentile,ftrackmult08);
		hpTLvsRefMult08vsDphi->Fill(pt_leading,ftrackmult08,DPhi);
		hpTLvsV0MmultvsDphi->Fill(pt_leading,fv0mpercentile,DPhi);

		vals[0]=pt;
		vals[1]=pt_leading;
		vals[2]=fv0mpercentile;
		vals[3]=ftrackmult08;
		vals[4]=DPhi;
		vals[5]=DeltaEta;

		hptLvsv0MvsRefMultvsDphivsDeta->Fill(vals);

		// near side
		if(TMath::Abs(DPhi)<pi/3.0){
			mult_ns++;
			hDphiNS->Fill(DPhi);
		}
		else if(TMath::Abs(DPhi-pi)<pi/3.0){
			mult_as++;
			hDphiAS->Fill(DPhi);
		}
		else{
			mult_ts++;
			hDphiTS->Fill(DPhi);
		}


	}// end loop over tracks

	hpTLvsNch->Fill(pt_leading,mult);
	hpTLvsRefMult08->Fill(pt_leading,ftrackmult08);
	hpTLvsV0Mmult->Fill(pt_leading,fv0mpercentile);
	hpTLvsV0MmultvsRefMult08->Fill(pt_leading,fv0mpercentile,ftrackmult08);
       
		
	// areas
	Double_t total_a = 2*TMath::Pi()*1.6;
	Double_t ns_a = 2*(TMath::Pi()/3.0)*1.6;
	Double_t as_a = 2*(TMath::Pi()/3.0)*1.6;
	Double_t ts_a = total_a-(ns_a+as_a);

	ProfpTLvsNchNS->Fill(pt_leading,mult_ns/ns_a);
	ProfpTLvsNchAS->Fill(pt_leading,mult_as/as_a);
	ProfpTLvsNchTS->Fill(pt_leading,mult_ts/ts_a);

	hpTLvsNchNS->Fill(pt_leading,1.0*mult_ns);
	hpTLvsNchAS->Fill(pt_leading,1.0*mult_as);
	hpTLvsNchTS->Fill(pt_leading,1.0*mult_ts);
}
//____________________________________________________________
void AliAnalysisTaskUeMultDep::SetTrackCuts(AliAnalysisFilter* fTrackFilter){

	AliESDtrackCuts* esdTrackCuts = new AliESDtrackCuts;
	// TPC
	esdTrackCuts->SetMinNCrossedRowsTPC(70);
	esdTrackCuts->SetMinRatioCrossedRowsOverFindableClustersTPC(0.8);

	esdTrackCuts->SetMaxChi2PerClusterTPC(4);
	esdTrackCuts->SetAcceptKinkDaughters(kFALSE);
	esdTrackCuts->SetRequireTPCRefit(kTRUE);
	// ITS
	esdTrackCuts->SetRequireITSRefit(kTRUE);
	esdTrackCuts->SetClusterRequirementITS(AliESDtrackCuts::kSPD,
			AliESDtrackCuts::kAny);
	// 7*(0.0015+0.0050/pt^1.1)
	esdTrackCuts->SetMaxDCAToVertexXYPtDep("0.0105+0.0350/pt^1.1");

	esdTrackCuts->SetMaxDCAToVertexZ(2);
	esdTrackCuts->SetDCAToVertex2D(kFALSE);
	esdTrackCuts->SetRequireSigmaToVertex(kFALSE);

	esdTrackCuts->SetMaxChi2PerClusterITS(36);

	/*
	   AliESDtrackCuts* esdTrackCuts = new AliESDtrackCuts();
	   esdTrackCuts->SetMaxFractionSharedTPCClusters(0.4);//
	   esdTrackCuts->SetMinRatioCrossedRowsOverFindableClustersTPC(0.8);//
	   esdTrackCuts->SetCutGeoNcrNcl(3., 130., 1.5, 0.85, 0.7);//
	   esdTrackCuts->SetMaxChi2PerClusterTPC(4);//
	   esdTrackCuts->SetAcceptKinkDaughters(kFALSE);//
	   esdTrackCuts->SetRequireTPCRefit(kTRUE);//
	   esdTrackCuts->SetRequireITSRefit(kTRUE);//
	   esdTrackCuts->SetClusterRequirementITS(AliESDtrackCuts::kSPD,
	   AliESDtrackCuts::kAny);//
	   esdTrackCuts->SetMaxDCAToVertexXYPtDep("0.0182+0.0350/pt^1.01");//
	   esdTrackCuts->SetMaxChi2TPCConstrainedGlobal(36);//
	   esdTrackCuts->SetMaxDCAToVertexZ(2);//
	   esdTrackCuts->SetDCAToVertex2D(kFALSE);//
	   esdTrackCuts->SetRequireSigmaToVertex(kFALSE);//
	   esdTrackCuts->SetMaxChi2PerClusterITS(36);//
	 */
	fTrackFilter->AddCuts(esdTrackCuts);

}

Double_t AliAnalysisTaskUeMultDep::DeltaPhi(Double_t phia, Double_t phib,
		Double_t rangeMin, Double_t rangeMax)
{
	Double_t dphi = -999;
	Double_t pi = TMath::Pi();

	if (phia < 0)         phia += 2*pi;
	else if (phia > 2*pi) phia -= 2*pi;
	if (phib < 0)         phib += 2*pi;
	else if (phib > 2*pi) phib -= 2*pi;
	dphi = phib - phia;
	if (dphi < rangeMin)      dphi += 2*pi;
	else if (dphi > rangeMax) dphi -= 2*pi;

	return dphi;
}
