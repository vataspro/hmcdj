#include <Grid/Grid.h>
#include <hmcdj/hmcdj.h>

/*
 * MAIN
 */
int main(int argc, char* argv[]) {
  /* typedefs */
  typedef Grid::Representations<Grid::SpFundamentalRepresentation>
      TheRepresentations;
  typedef Grid::SpWilsonImplR FermionImplPolicy;
  typedef Grid::SpWilsonFermionD FermionAction;
  typedef typename FermionAction::FermionField FermionField;

  typedef Grid::GenericSpHMCRunnerHirep<TheRepresentations, Grid::MinimumNorm2>
      HMCWrapper;

  // HMCDJ Parameters
  auto beta = djParameter<double>("beta");
  auto mass = djParameter<double>("mass");
  djParameterList params = {beta, mass};

  // Initialise HMCDJ
  DJ<HMCWrapper> hmcdj("TwoFundamentalFermions", argc, argv, params);

  // Print the layout
  Grid::GridLogLayout();

  /* Action */
  Grid::SpWilsonGaugeActionR Waction(beta);

  auto GridPtr = hmcdj.TheHMC.Resources.GetCartesian();
  auto GridRBPtr = hmcdj.TheHMC.Resources.GetRBCartesian();

  Grid::SpFundamentalRepresentation::LatticeField U(GridPtr);

  /* Observables */
  // Add the Plaquette observable
  typedef Grid::PlaquetteMod<HMCWrapper::ImplPolicy> PlaqObs;
  hmcdj.TheHMC.Resources.AddObservable<PlaqObs>();
  // Add the temporal Polyakov Loop observable
  typedef Grid::PolyakovMod<HMCWrapper::ImplPolicy> PolyakovObs;
  hmcdj.TheHMC.Resources.AddObservable<PolyakovObs>();

  // Boundary conditions input  by hand right now, should be fixed
  // with allowing integer params
  // THIS BC IMPLEMENTS FINITE TEMPERATURE PHYSICS
  std::vector<Grid::ComplexD> boundary = {1, 1, 1, -1};
  FermionAction::ImplParams bc(boundary);

  FermionAction FermOp(U, *GridPtr, *GridRBPtr, mass, bc);

  Grid::ConjugateGradient<FermionField> CG(1.0e-8, 2000, false);

  Grid::TwoFlavourPseudoFermionAction<FermionImplPolicy> Nf2(FermOp, CG, CG);

  Nf2.is_smeared = false;

  Grid::ActionLevel<HMCWrapper::Field, TheRepresentations> Level1(1);
  Level1.push_back(&Nf2);

  Grid::ActionLevel<HMCWrapper::Field, TheRepresentations> Level2(4);
  Level2.push_back(&Waction);

  hmcdj.TheHMC.TheAction.push_back(Level1);
  hmcdj.TheHMC.TheAction.push_back(Level2);

  /* RUN THE HMC */
  hmcdj.Play();

  return 0;
}
