#include <hmcdj/utils/dj.h>
#include <Grid/Grid.h>

/* 
 * MAIN
 */
int main(int argc, char* argv[]) {

    typedef Grid::GenericSpHMCRunner<Grid::MinimumNorm2> HMCWrapper;

    DJ<HMCWrapper> hmcdj(argc, argv);

    /* Action */
    Grid::RealD beta = 6.9;
    Grid::SpWilsonGaugeActionR Waction(beta);
  
    Grid::ActionLevel<HMCWrapper::Field> Level1(1);
    Level1.push_back(&Waction);
    hmcdj.TheHMC.TheAction.push_back(Level1);

    // TheHMC.ReadCommandLine(argc, argv); // these can be parameters from file
    hmcdj.Play();

    Grid::Grid_finalize();  

    return 0;
}