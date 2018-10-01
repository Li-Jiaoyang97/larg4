////////////////////////////////////////////////////////////////////////
/// \file  ParticleListAction_service.h
/// \brief Use Geant4's user "hooks" to maintain a list of particles generated by Geant4.
///
/// \author  seligman@nevis.columbia.edu
////////////////////////////////////////////////////////////////////////
//
// accumulate a list of particles modeled by Geant4.
//

#ifndef PARTICLELISTACTION_SERVICE_H
#define PARTICLELISTACTION_SERVICE_H
// Includes
//#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art/Framework/Core/ProductRegistryHelper.h"
#include "art/Framework/Services/Registry/ServiceMacros.h"
#include "art/Persistency/Provenance/ModuleDescription.h"

#include "larsim/LArG4/ParticleFilters.h" // larg4::PositionInVolumeFilter
#include "nutools/ParticleNavigation/ParticleList.h" // larg4::PositionInVolumeFilter
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "nusimdata/SimulationBase/simb.h" // simb::GeneratedParticleIndex_t
// Get the base classes
#include "artg4tk/actionBase/EventActionBase.hh"
#include "artg4tk/actionBase/TrackingActionBase.hh"
#include "artg4tk/actionBase/SteppingActionBase.hh"
#
#
#include "Geant4/globals.hh"
#include <map>

// Forward declarations.
class G4Event;
class G4Track;
class G4Step;

namespace sim { 
  class ParticleList; 
}

namespace larg4 {

  class ParticleListActionService : public  artg4tk::EventActionBase, 
                                    public  artg4tk::TrackingActionBase,
                                    public  artg4tk::SteppingActionBase,
    private art::ProductRegistryHelper
  {
  public:

    struct ParticleInfo_t {
      
      simb::MCParticle* particle = nullptr;  ///< simple structure representing particle
      bool              keep = false;        ///< if there was decision to keep
      /// Index of the particle in the original generator truth record.
      simb::GeneratedParticleIndex_t truthIndex = simb::NoGeneratedParticleIndex;
      /// Resets the information (does not release memory it does not own)
      void clear()
      { particle = nullptr; 
	keep = false;  
	truthIndex = simb::NoGeneratedParticleIndex;
      }
 
      /// Returns whether there is a particle
      bool hasParticle() const { return particle; }

       /// Returns whether there is a particle
      bool isPrimary() const { return simb::isGeneratedParticleIndex(truthIndex); }
      
      /// Rerturns whether there is a particle known to be kept
      bool keepParticle() const { return hasParticle() && keep; }

      /// Returns the index of the particle in the generator truth record.
      simb::GeneratedParticleIndex_t truthInfoIndex() const { return truthIndex; }
      
    }; // ParticleInfo_t
    using art::ProductRegistryHelper::produces;
    using art::ProductRegistryHelper::registerProducts;
 
 
    // Standard constructors and destructors;
    ParticleListActionService(fhicl::ParameterSet const&, 
			      art::ActivityRegistry&);
    //			      double energyCut, 
    //			      bool storeTrajectories=false, 
    //			      bool keepEMShowerDaughters=false);
    virtual ~ParticleListActionService();

    // UserActions method that we'll override, to obtain access to
    // Geant4's particle tracks and trajectories.
    virtual void preUserTrackingAction (const G4Track*) override;
    virtual void postUserTrackingAction(const G4Track*) override;
    virtual void userSteppingAction(const G4Step* ) override;

    /// Grabs a particle filter
    void ParticleFilter(std::unique_ptr<PositionInVolumeFilter>&& filter)
      { fFilter = std::move(filter); }

    // TrackID of the current particle, EveID if the particle is from an EM shower
    static int               GetCurrentTrackID() { return fCurrentTrackID; }
			                                                     
    void                     ResetTrackIDOffset() { fTrackIDOffset = 0;     }

    // Returns the ParticleList accumulated during the current event.
    const sim::ParticleList* GetList() const;
        
    /// Returns a map of truth record information index for each of the primary
    /// particles (by track ID).
    std::map<int, simb::GeneratedParticleIndex_t> const& GetPrimaryTruthMap() const
      { return fPrimaryTruthMap; }
    
    /// Returns the index of primary truth (`sim::NoGeneratorIndex` if none).
    simb::GeneratedParticleIndex_t GetPrimaryTruthIndex(int trackId) const;
    
    // Yields the ParticleList accumulated during the current event.
    sim::ParticleList&& YieldList();

    /// returns whether the specified particle has been marked as dropped
    static bool isDropped(simb::MCParticle const* p);

    // Called at the beginning of each event (note that this is after the
    // primaries have been generated and sent to the event manager)
    void beginOfEventAction(const G4Event* ) override;

    // Called at the end of each event, right before GEANT's state switches
    // out of event processing and into closed geometry (last chance to access
    // the current event).
    void endOfEventAction(const G4Event* ) override;
    // Set/get the current Art event
    void setCurrArtEvent(art::Event & e) { currentArtEvent_ = &e; }
    art::Event  *getCurrArtEvent();
    void  setProductID(art::ProductID pid){pid_=pid;}
    std::unique_ptr <std::vector<simb::MCParticle>>  &GetParticleCollection(){return partCol_;}
    std::unique_ptr <art::Assns<simb::MCTruth, simb::MCParticle >> &GetAssnsMCTruthToMCParticle(){return tpassn_;}
  private:

    // this method will loop over the fParentIDMap to get the 
    // parentage of the provided trackid
    int               	     GetParentage(int trackid) const;

    G4double                 fenergyCut;             ///< The minimum energy for a particle to 		     	  
                                                     ///< be included in the list.
    ParticleInfo_t           fCurrentParticle;       ///< information about the particle currently being simulated
                                                     ///< for a single particle.		
    sim::ParticleList*       fparticleList;          ///< The accumulated particle information for 
                                                     ///< all particles in the event.	
    G4bool                   fstoreTrajectories;     ///< Whether to store particle trajectories with each particle. 
    std::map<int, int>       fParentIDMap;           ///< key is current track ID, value is parent ID		  
    static int               fCurrentTrackID;        ///< track ID of the current particle, set to eve ID 
                                                     ///< for EM shower particles		
    static int               fTrackIDOffset;         ///< offset added to track ids when running over		  
                                                     ///< multiple MCTruth objects.				  
    bool                     fKeepEMShowerDaughters; ///< whether to keep EM shower secondaries, tertiaries, etc     
    
    std::unique_ptr<PositionInVolumeFilter> fFilter; ///< filter for particles to be kept
    
    /// Map: particle track ID -> index of primary information in MC truth.
    std::map<int, simb::GeneratedParticleIndex_t> fPrimaryTruthMap;

    // Hold on to the current Art event
    art::Event * currentArtEvent_;

    std::unique_ptr<std::vector<simb::MCParticle> > partCol_;
    std::unique_ptr<art::Assns<simb::MCTruth, simb::MCParticle >> tpassn_;
    art::ProductID pid_;
    /// Adds a trajectory point to the current particle, and runs the filter
    void AddPointToCurrentParticle(TLorentzVector const& pos,
                                   TLorentzVector const& mom,
                                   std::string    const& process);
  };

} // namespace larg4
using larg4::ParticleListActionService;
DECLARE_ART_SERVICE(ParticleListActionService,LEGACY)
#endif // PARTICLELISTACTION_SERVICE_H
