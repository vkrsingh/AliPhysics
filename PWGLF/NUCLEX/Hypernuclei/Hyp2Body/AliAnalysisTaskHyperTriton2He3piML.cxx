#include "AliAnalysisTaskHyperTriton2He3piML.h"

#include <array>
#include <unordered_map>
#include <TRandom3.h>
#include <Riostream.h>
#include <TChain.h>
#include <TFile.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TList.h>
#include <TMath.h>
#include <stdio.h>
#include "AliAnalysisManager.h"
#include "AliESDEvent.h"
#include "AliESDtrack.h"
#include "AliExternalTrackParam.h"
#include "AliInputEventHandler.h"
#include "AliMCEvent.h"
#include "AliPID.h"
#include "AliPDG.h"
#include "AliPIDResponse.h"
#include "AliVVertex.h"

using std::cout;
using std::endl;

ClassImp(AliAnalysisTaskHyperTriton2He3piML);

namespace
{
constexpr double Sq(double x) { return x * x; }
constexpr float kEps = 1.e-6;

int ComputeMother(AliMCEvent *mcEvent, const AliESDtrack *one, const AliESDtrack *two)
{
  int labOne = std::abs(one->GetLabel());
  int labTwo = std::abs(two->GetLabel());

  if (mcEvent->IsPhysicalPrimary(labOne) ||
      mcEvent->IsPhysicalPrimary(labTwo) ||
      mcEvent->IsSecondaryFromMaterial(labOne) ||
      mcEvent->IsSecondaryFromMaterial(labTwo))
    return -1;
  else
  {
    AliVParticle *partOne = mcEvent->GetTrack(labOne);
    AliVParticle *partTwo = mcEvent->GetTrack(labTwo);
    if (partOne->GetMother() != partTwo->GetMother())
    {
      return -1;
    }
    else
    {
      if (one->GetLabel() * two->GetLabel() >= 0)
        return partTwo->GetMother();
      else
        return -partTwo->GetMother();
    }
  }
}

} // namespace

AliAnalysisTaskHyperTriton2He3piML::AliAnalysisTaskHyperTriton2He3piML(
    bool mc, std::string name)
    : AliAnalysisTaskSE(name.data()),
      fEventCuts{},
      fListHist{nullptr},
      fTreeV0{nullptr},
      fPIDResponse{nullptr},
      fMC{mc},
      fUseOnTheFly{false},
      fUseCustomBethe{false},
      fCustomBethe{0.f, 0.f, 0.f, 0.f, 0.f},
      fCustomResolution{1.f},
      fHistNsigmaHe3{nullptr},
      fHistNsigmaPion{nullptr},
      fHistInvMass{nullptr},
      fMinPtToSave{0.1},
      fMaxPtToSave{100},
      fMaxTPCpiSigma{10.},
      fMaxTPChe3Sigma{10.},
      fSHyperTriton{},
      fRHyperTriton{},
      fRCollision{}
{
  // Standard output
  DefineInput(0, TChain::Class());
  DefineOutput(1, TList::Class()); // Basic Histograms
  DefineOutput(2, TTree::Class()); // V0 Tree output
}

AliAnalysisTaskHyperTriton2He3piML::~AliAnalysisTaskHyperTriton2He3piML()
{
  if (fListHist)
  {
    delete fListHist;
    fListHist = nullptr;
  }

  if (fTreeV0)
  {
    delete fTreeV0;
    fTreeV0 = nullptr;
  }
}

void AliAnalysisTaskHyperTriton2He3piML::UserCreateOutputObjects()
{
  AliAnalysisManager *man = AliAnalysisManager::GetAnalysisManager();
  AliInputEventHandler *inputHandler = (AliInputEventHandler *)(man->GetInputEventHandler());
  fPIDResponse = inputHandler->GetPIDResponse();
  inputHandler->SetNeedField();

  fTreeV0 = new TTree("fTreeV0", "V0 Candidates");
  fTreeV0->Branch("RCollision", &fRCollision);
  fTreeV0->Branch("RHyperTriton", &fRHyperTriton);

  if (man->GetMCtruthEventHandler())
    fTreeV0->Branch("SHyperTriton", &fSHyperTriton);

  fListHist = new TList();
  fListHist->SetOwner();
  fEventCuts.AddQAplotsToList(fListHist);

  fHistNsigmaPion =
      new TH2D("fHistNsigmaPosPion", ";#it{p}_{T} (GeV/#it{c});n_{#sigma} TPC Pion; Counts", 100, 0, 10, 20, 0, 10);
  fHistNsigmaHe3 =
      new TH2D("fHistNsigmaNegHe3", ";#it{p}_{T} (GeV/#it{c});n_{#sigma} TPC ^{3}He; Counts", 100, 0, 10, 20, 0, 10);
  fHistInvMass =
      new TH2D("fHistInvMass", ";#it{p}_{T} (GeV/#it{c});Invariant Mass(GeV/#it{c^2}); Counts", 100, 0, 10, 30, 2.96, 3.05);

  fListHist->Add(fHistNsigmaPion);
  fListHist->Add(fHistNsigmaHe3);
  fListHist->Add(fHistInvMass);

  PostData(1, fListHist);
  PostData(2, fTreeV0);

  AliPDG::AddParticlesToPdgDataBase();
} // end UserCreateOutputObjects

void AliAnalysisTaskHyperTriton2He3piML::UserExec(Option_t *)
{
  AliESDEvent *esdEvent = dynamic_cast<AliESDEvent *>(InputEvent());
  if (!esdEvent)
  {
    ::Fatal("AliAnalysisTaskHyperTriton2He3piML::UserExec",
            "AliESDEvent not found.");
    return;
  }

  AliMCEvent *mcEvent = MCEvent();
  if (!mcEvent && fMC)
  {
    ::Fatal("AliAnalysisTaskHyperTriton2He3piML::UserExec", "Could not retrieve MC event");
    return;
  }

  if (!fEventCuts.AcceptEvent(esdEvent))
  {
    PostData(1, fListHist);
    PostData(2, fTreeV0);
    return;
  }

  double primaryVertex[3];
  fRCollision.fCent = fEventCuts.GetCentrality();
  fEventCuts.GetPrimaryVertex()->GetXYZ(primaryVertex);

  fRCollision.fX = primaryVertex[0];
  fRCollision.fY = primaryVertex[1];
  fRCollision.fZ = primaryVertex[2];

  std::unordered_map<int, int> mcMap;
  if (fMC)
  {
    fSHyperTriton.clear();
    for (int ilab = 0; ilab < mcEvent->GetNumberOfTracks(); ilab++)
    { // This is the begining of the loop on tracks
      AliVParticle *part = mcEvent->GetTrack(ilab);
      if (!part)
      {
        ::Warning("AliAnalysisTaskHyperTriton2He3piML::UserExec", "Generated loop %d - MC TParticle pointer to current stack particle = 0x0 ! Skipping.", ilab);
        continue;
      }

      int currentPDG = part->PdgCode();
      if (std::abs(currentPDG) == 1010010030)
      {
        if (std::abs(part->Y()) > 1.)
          continue;

        double sVtx[3]{0.0, 0.0, 0.0};

        AliVParticle *he3{nullptr}, *pi{nullptr};
        for (int iD = part->GetDaughterFirst(); iD <= part->GetDaughterLast(); ++iD)
        {
          AliVParticle *dau = mcEvent->GetTrack(iD);

          if (mcEvent->IsSecondaryFromWeakDecay(iD) && dau)
          {
            sVtx[0] = dau->Xv();
            sVtx[1] = dau->Yv();
            sVtx[2] = dau->Zv();

            if (std::abs(dau->PdgCode()) == AliPID::ParticleCode(AliPID::kHe3))
              he3 = dau;
            if (std::abs(dau->PdgCode()) == AliPID::ParticleCode(AliPID::kPion))
              pi = dau;
          }
        }
        if (he3 == nullptr)
          continue;

        SHyperTritonHe3pi v0part;
        v0part.fPdgCode = currentPDG;
        v0part.fDecayX = sVtx[0];
        v0part.fDecayY = sVtx[1];
        v0part.fDecayZ = sVtx[2];
        v0part.fPxHe3 = he3->Px();
        v0part.fPyHe3 = he3->Py();
        v0part.fPzHe3 = he3->Pz();
        v0part.fPxPi = pi->Px();
        v0part.fPyPi = pi->Py();
        v0part.fPzPi = pi->Pz();
        v0part.fFake = true;
        mcMap[ilab] = fSHyperTriton.size();
        fSHyperTriton.push_back(v0part);
      }
    }
  }

  fRHyperTriton.clear();
  for (int iV0 = 0; iV0 < esdEvent->GetNumberOfV0s();
       iV0++)
  { // This is the begining of the V0 loop (we analyse only offline
    // V0s)
    AliESDv0 *v0 = ((AliESDEvent *)esdEvent)->GetV0(iV0);
    if (!v0)
      continue;
    if (v0->GetOnFlyStatus() != 0 && !fUseOnTheFly)
      continue;
    if (fUseOnTheFly && v0->GetOnFlyStatus() == 0)
      continue;

    // Remove like-sign (will not affect offline V0 candidates!)
    if (v0->GetParamN()->Charge() * v0->GetParamP()->Charge() > 0)
      continue;

    const int lKeyPos = std::abs(v0->GetPindex());
    const int lKeyNeg = std::abs(v0->GetNindex());
    AliESDtrack *pTrack = esdEvent->GetTrack(lKeyPos);
    AliESDtrack *nTrack = esdEvent->GetTrack(lKeyNeg);

    if (!pTrack || !nTrack)
      ::Fatal("AliAnalysisTaskHyperTriton2He3piML::UserExec",
              "Could not retreive one of the daughter track");

    // Filter like-sign V0 (next: add counter and distribution)
    if (pTrack->GetSign() == nTrack->GetSign())
      continue;

    if (std::abs(nTrack->Eta()) > 0.8 || std::abs(pTrack->Eta()) > 0.8)
      continue;

    // TPC refit condition (done during reconstruction for Offline but not for
    // On-the-fly)
    if (!(pTrack->GetStatus() & AliESDtrack::kTPCrefit))
      continue;
    if (!(nTrack->GetStatus() & AliESDtrack::kTPCrefit))
      continue;

    // GetKinkIndex condition
    if (pTrack->GetKinkIndex(0) > 0 || nTrack->GetKinkIndex(0) > 0)
      continue;

    // Findable cluster s > 0 condition
    if (pTrack->GetTPCNclsF() <= 0 || nTrack->GetTPCNclsF() <= 0)
      continue;

    // Official means of acquiring N-sigmas
    float nSigmaPosPi = fPIDResponse->NumberOfSigmasTPC(pTrack, AliPID::kPion);
    float nSigmaPosHe3 = fPIDResponse->NumberOfSigmasTPC(pTrack, AliPID::kHe3);
    float nSigmaNegPi = fPIDResponse->NumberOfSigmasTPC(nTrack, AliPID::kPion);
    float nSigmaNegHe3 = fPIDResponse->NumberOfSigmasTPC(nTrack, AliPID::kHe3);

    if (fUseCustomBethe)
    {
      const float nbetaGamma = nTrack->GetTPCmomentum() / AliPID::ParticleMass(AliPID::kHe3);
      const float pbetaGamma = pTrack->GetTPCmomentum() / AliPID::ParticleMass(AliPID::kHe3);
      const float *pars = fCustomBethe;
      const float nExpSignal = AliExternalTrackParam::BetheBlochAleph(nbetaGamma, pars[0], pars[1], pars[2], pars[3], pars[4]);
      const float pExpSignal = AliExternalTrackParam::BetheBlochAleph(pbetaGamma, pars[0], pars[1], pars[2], pars[3], pars[4]);
      nSigmaNegHe3 = (nTrack->GetTPCsignal() - nExpSignal) / (fCustomResolution * nExpSignal);
      nSigmaPosHe3 = (pTrack->GetTPCsignal() - pExpSignal) / (fCustomResolution * pExpSignal);
    }

    const float nSigmaNegAbsHe3 = std::abs(nSigmaNegHe3);
    const float nSigmaPosAbsHe3 = std::abs(nSigmaPosHe3);
    const float nSigmaNegAbsPi = std::abs(nSigmaNegPi);
    const float nSigmaPosAbsPi = std::abs(nSigmaPosPi);

    /// Skip V0 not involving (anti-)He3 candidates

    bool mHyperTriton = nSigmaPosAbsHe3 < fMaxTPChe3Sigma && nSigmaNegAbsPi < fMaxTPCpiSigma;
    bool aHyperTriton = nSigmaNegAbsHe3 < fMaxTPChe3Sigma && nSigmaPosAbsPi < fMaxTPCpiSigma;
    if (!mHyperTriton && !aHyperTriton)
      continue;

    AliESDtrack *he3Track = aHyperTriton ? nTrack : pTrack;
    AliESDtrack *piTrack = he3Track == nTrack ? pTrack : nTrack;

    double pP[3]{0.0}, nP[3]{0.0};
    v0->GetPPxPyPz(pP[0], pP[1], pP[2]);
    v0->GetNPxPyPz(nP[0], nP[1], nP[2]);
    const double *he3P = (he3Track == pTrack) ? pP : nP;
    const double *piP = (piTrack == pTrack) ? pP : nP;

    LVector_t he3Vector, piVector, hyperVector;
    he3Vector.SetCoordinates(2 * he3P[0], 2 * he3P[1], 2 * he3P[2], AliPID::ParticleMass(AliPID::kHe3));
    piVector.SetCoordinates(piP[0], piP[1], piP[2], AliPID::ParticleMass(AliPID::kPion));
    hyperVector = piVector + he3Vector;
    
    float v0Pt = hyperVector.Pt();
    if ((v0Pt < fMinPtToSave) || (fMaxPtToSave < v0Pt))
      continue;

    if (hyperVector.M() < 2.8|| hyperVector.M() > 3.2)
      continue;
    // Track quality cuts

    float he3B[2], piB[2], bCov[3];
    piTrack->GetImpactParameters(piB, bCov);
    he3Track->GetImpactParameters(he3B, bCov);
    const float he3DCA = std::hypot(he3B[0], he3B[1]);
    const float piDCA = std::hypot(piB[0], piB[1]);

    if ((pTrack->GetTPCClusterInfo(2, 1) < 70) ||
        (nTrack->GetTPCClusterInfo(2, 1) < 70))
      continue;

    unsigned char posXedRows = pTrack->GetTPCClusterInfo(2, 1);
    unsigned char negXedRows = nTrack->GetTPCClusterInfo(2, 1);
    float posChi2PerCluster =
        pTrack->GetTPCchi2() / (pTrack->GetTPCNcls() + 1.e-16);
    float negChi2PerCluster =
        nTrack->GetTPCchi2() / (nTrack->GetTPCNcls() + 1.e-16);
    float posXedRowsOverFindable = float(posXedRows) / pTrack->GetTPCNclsF();
    float negXedRowsOverFindable = float(negXedRows) / nTrack->GetTPCNclsF();
    unsigned char minXedRows =
        posXedRows < negXedRows ? posXedRows : negXedRows;
    float minXedRowsOverFindable =
        posXedRowsOverFindable < negXedRowsOverFindable
            ? posXedRowsOverFindable
            : negXedRowsOverFindable;
    float maxChi2PerCluster = posChi2PerCluster > negChi2PerCluster
                                  ? posChi2PerCluster
                                  : negChi2PerCluster;

    // Filling the V0 vector
    int ilab = -1;
    if (fMC)
    {
      ilab = std::abs(ComputeMother(mcEvent, he3Track, piTrack));
      AliVParticle *part = mcEvent->GetTrack(ilab);
      if (part)
      {
        if (std::abs(part->PdgCode()) == 1010010030)
        {
          fSHyperTriton[mcMap[ilab]].fRecoIndex = (fRHyperTriton.size());
          fSHyperTriton[mcMap[ilab]].fFake = false;
        }
      }
    }

    double x{0.}, y{0.}, z{0.};
    v0->GetXYZ(x, y, z);

    RHyperTritonHe3pi v0part;
    v0part.fDecayX = x - fRCollision.fX;
    v0part.fDecayY = y - fRCollision.fY;
    v0part.fDecayZ = z - fRCollision.fZ;
    v0part.fPxHe3 = he3Vector.Px();
    v0part.fPyHe3 = he3Vector.Py();
    v0part.fPzHe3 = he3Vector.Pz();
    v0part.fPxPi = piVector.Px();
    v0part.fPyPi = piVector.Py();
    v0part.fPzPi = piVector.Pz();
    v0part.fDcaHe32PrimaryVertex = he3DCA;
    v0part.fDcaPi2PrimaryVertex = piDCA;
    v0part.fDcaV0daughters = v0->GetDcaV0Daughters();
    v0part.fLeastXedOverFindable = minXedRowsOverFindable;
    v0part.fMaxChi2PerCluster = maxChi2PerCluster;
    v0part.fTPCnSigmaHe3 = (pTrack == he3Track) ? nSigmaPosHe3 : nSigmaNegHe3;
    v0part.fTPCnSigmaPi = (pTrack == piTrack) ? nSigmaPosPi : nSigmaNegPi;
    v0part.fLeastNxedRows = minXedRows;
    v0part.fLeastNpidClusters = pTrack->GetTPCsignalN() > nTrack->GetTPCsignalN() ? nTrack->GetTPCsignalN() : pTrack->GetTPCsignalN();
    v0part.fITSrefitHe3 = he3Track->GetStatus() & AliESDtrack::kITSrefit;
    v0part.fITSrefitPi = piTrack->GetStatus() & AliESDtrack::kITSrefit;
    v0part.fSPDanyHe3 = he3Track->HasPointOnITSLayer(0) || he3Track->HasPointOnITSLayer(1);
    v0part.fSPDanyPi = piTrack->HasPointOnITSLayer(0) || piTrack->HasPointOnITSLayer(1);
    v0part.fMatter = (pTrack == he3Track);
    fRHyperTriton.push_back(v0part);

    fHistNsigmaPion->Fill(piTrack->Pt(), v0part.fTPCnSigmaPi);
    fHistNsigmaHe3->Fill(piTrack->Pt(), v0part.fTPCnSigmaHe3);
    fHistInvMass->Fill(hyperVector.Pt(), hyperVector.M());
  }

  fTreeV0->Fill();

  PostData(1, fListHist);
  PostData(2, fTreeV0);
}

void AliAnalysisTaskHyperTriton2He3piML::Terminate(Option_t *) {}

void AliAnalysisTaskHyperTriton2He3piML::SetCustomBetheBloch(float res, const float *bethe)
{
  fUseCustomBethe = true;
  fCustomResolution = res;
  std::copy(bethe, bethe + 5, fCustomBethe);
}
