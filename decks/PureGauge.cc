#include <Grid/Grid.h>
#include <hmcdj/hmcdj.h>

/*
 * MAIN
 */
int main(int argc, char* argv[]) {
  typedef Grid::GenericSpHMCRunner<Grid::MinimumNorm2> HMCWrapper;

  // Define parameters
  auto beta = djParameter<double>("beta");
  djParameterList params = {beta};

  DJ<HMCWrapper> hmcdj("PureGauge", argc, argv, params);

  /* Observables */
  // Add the Plaquette observable
  typedef Grid::PlaquetteMod<HMCWrapper::ImplPolicy> PlaqObs;
  hmcdj.TheHMC.Resources.AddObservable<PlaqObs>();
  // Add the temporal Polyakov Loop observable
  typedef Grid::PolyakovMod<HMCWrapper::ImplPolicy> PolyakovObs;
  hmcdj.TheHMC.Resources.AddObservable<PolyakovObs>();

  /* Action */
  Grid::SpWilsonGaugeActionR Waction(beta);

  Grid::ActionLevel<HMCWrapper::Field> Level1(1);
  Level1.push_back(&Waction);
  hmcdj.TheHMC.TheAction.push_back(Level1);

  hmcdj.Tune();

  // for (auto v : *hmcdj.AccPar.AcceptanceArray) {
  //   std::cout << Grid::GridLogMessage << v << std::endl;
  // }

  // hmcdj.Play();

  return 0;
}
