/*
decks/TwoFundamentalMobiusFermions.cc

Deck for implementing two fermions in the fundamental
representation with Sp gauge group.

*/

#include <Grid/Grid.h>
#include <hmcdj/hmcdj.h>

/*
 * MAIN
 */
int main(int argc, char *argv[]) {
  /* typedefs */
  typedef Grid::Representations<Grid::SpFundamentalRepresentation>
      TheRepresentations;

  typedef Grid::SpWilsonImplR FermionImplPolicy;

  typedef Grid::MobiusFermion<Grid::SpWilsonImplD> FermionAction;

  typedef typename FermionAction::FermionField FermionField;

  typedef Grid::GenericSpHMCRunnerHirep<TheRepresentations, Grid::MinimumNorm2>
      HMCWrapper;

  // Hardcoded parameter
  Grid::RealD pv_mass = 1.0;

  // HMCDJ Parameters
  auto beta = djParameter<double>("beta");
  auto mass = djParameter<double>("mass");
  // Mobius parameters
  auto M5 = djParameter<double>("M5");
  auto b = djParameter<double>("b");
  auto c = djParameter<double>("c");
  auto Ls = djParameter<int>("Ls");
  // To parameter list
  djParameterList params = {beta, mass, M5, b, c, Ls};

  // Initialise HMCDJ
  DJ<HMCWrapper> hmcdj("TwoFundamentalMobiusFermions", argc, argv, params);

  // Print the layout
  Grid::GridLogLayout();

  /* Action */
  Grid::SpWilsonGaugeActionR Waction(beta);

  /* Lattices */
  auto GridPtr = hmcdj.TheHMC.Resources.GetCartesian();
  auto GridRBPtr = hmcdj.TheHMC.Resources.GetRBCartesian();
  auto FGrid = Grid::SpaceTimeGrid::makeFiveDimGrid(Ls, GridPtr);
  auto FrbGrid = Grid::SpaceTimeGrid::makeFiveDimRedBlackGrid(Ls, GridPtr);

  /* Gauge Field */
  Grid::SpFundamentalRepresentation::LatticeField U(GridPtr);

  /* Observables */
  // Add the Plaquette observable -- already present in DWF implementation
  // typedef Grid::PlaquetteMod<HMCWrapper::ImplPolicy> PlaqObs;
  // hmcdj.TheHMC.Resources.AddObservable<PlaqObs>();
  // Add the temporal Polyakov Loop observable
  typedef Grid::PolyakovMod<HMCWrapper::ImplPolicy> PolyakovObs;
  hmcdj.TheHMC.Resources.AddObservable<PolyakovObs>();

  // Boundary conditions input  by hand right now, should be fixed
  // with allowing integer params
  // THIS BC IMPLEMENTS FINITE TEMPERATURE PHYSICS
  std::vector<Grid::ComplexD> boundary = {1, 1, 1, -1};
  FermionAction::ImplParams bc(boundary);

  double StoppingCondition = 1e-10;
  double MaxCGIterations = 3000;
  Grid::ConjugateGradient<FermionField> CG(StoppingCondition, MaxCGIterations);

  Grid::ActionLevel<HMCWrapper::Field, TheRepresentations> Level1(1);
  Grid::ActionLevel<HMCWrapper::Field, TheRepresentations> Level2(4);

  std::vector<FermionAction *> Numerators;
  std::vector<FermionAction *> Denominators;
  std::vector<
      Grid::TwoFlavourEvenOddRatioPseudoFermionAction<FermionImplPolicy> *>
      Quotients;

  Numerators.push_back(new FermionAction(U, *FGrid, *FrbGrid, *GridPtr,
                                         *GridRBPtr, pv_mass, M5, b, c, bc));

  Denominators.push_back(new FermionAction(U, *FGrid, *FrbGrid, *GridPtr,
                                           *GridRBPtr, mass, M5, b, c, bc));

  Quotients.push_back(
      new Grid::TwoFlavourEvenOddRatioPseudoFermionAction<FermionImplPolicy>(
          *Numerators[0], *Denominators[0], CG, CG));

  Level1.push_back(Quotients[0]);

  /////////////////////////////////////////////////////////////
  // Gauge action
  /////////////////////////////////////////////////////////////
  hmcdj.TheHMC.TheAction.push_back(Level1);
  hmcdj.TheHMC.TheAction.push_back(Level2);
  std::cout << Grid::GridLogMessage << " Action complete " << std::endl;

  /* RUN THE HMC */
  hmcdj.Play();

  return 0;
}
