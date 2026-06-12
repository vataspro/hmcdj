/*
  * decks/NoParams.cc

  This example deck has no custom hmcdj parameters
  and implements the
  overloaded constructor for the DJ class.

  It runs a Pure Gauge HMC chain with the value
  of beta hard coded below

*/
#include <Grid/Grid.h>
#include <hmcdj/hmcdj.h>

#define BETA_VAL 7.2

/*
 * MAIN
 */
int main(int argc, char* argv[]) {
  typedef Grid::GenericSpHMCRunner<Grid::MinimumNorm2> HMCWrapper;

  // Instantiate DJ
  const bool reducedStorage = true;
  DJ<HMCWrapper> hmcdj("NoParams", argc, argv, {}, reducedStorage);

  /* Observables */
  // Add the temporal Polyakov Loop observable
  typedef Grid::PolyakovMod<HMCWrapper::ImplPolicy> PolyakovObs;
  hmcdj.TheHMC.Resources.AddObservable<PolyakovObs>();

  /* Action */
  Grid::SpWilsonGaugeActionR Waction(BETA_VAL);

  Grid::ActionLevel<HMCWrapper::Field> Level1(1);
  Level1.push_back(&Waction);
  hmcdj.TheHMC.TheAction.push_back(Level1);

  hmcdj.Play();

  return 0;
}
