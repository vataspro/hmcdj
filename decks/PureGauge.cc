#include <Grid/Grid.h>
#include <hmcdj/hmcdj.h>

/*
 * MAIN
 */
int main(int argc, char* argv[]) {
  typedef Grid::GenericSpHMCRunner<Grid::MinimumNorm2> HMCWrapper;

  // Define parameters
  auto BetaParam = std::make_unique<djParameter<double>>("beta");
  std::vector<ParameterBase*> params = {BetaParam.get()};

  DJ<HMCWrapper> hmcdj(argc, argv, params);

  /* Observables */
  // Add the Plaquette observable
  typedef Grid::PlaquetteMod<HMCWrapper::ImplPolicy> PlaqObs;
  hmcdj.TheHMC.Resources.AddObservable<PlaqObs>();
  // Add the temporal Polyakov Loop observable
  typedef Grid::PolyakovMod<HMCWrapper::ImplPolicy> PolyakovObs;
  hmcdj.TheHMC.Resources.AddObservable<PolyakovObs>();

  /* Action */
  // Grid::RealD beta = hmcdj.reader.Parameters["beta"];
  Grid::RealD beta = BetaParam->value;

  Grid::SpWilsonGaugeActionR Waction(beta);

  Grid::ActionLevel<HMCWrapper::Field> Level1(1);
  Level1.push_back(&Waction);
  hmcdj.TheHMC.TheAction.push_back(Level1);

  hmcdj.Play();

  return 0;
}
