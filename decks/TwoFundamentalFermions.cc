//g++ --std=c++17 gridyaml.cpp -o test -I/opt/homebrew/Cellar/yaml-cpp/0.8.0/include/ -L/opt/homebrew/Cellar/yaml-cpp/0.8.0/lib -I/Users/alexi/Work/phd/GRID/prefix_grid_202410/include -L/Users/alexi/Work/phd/GRID/prefix_grid_202410/lib -I/Users/alexi/openssl/include -L/Users/alexi/openssl/lib -lGrid  -lyaml-cpp -lz
#include <hmcdj/utils/dj.h>
#include <Grid/Grid.h>

/* 
 * MAIN
 */
int main(int argc, char* argv[]) {


    /* typedefs */
    typedef Grid::Representations<Grid::SpFundamentalRepresentation> TheRepresentations;
    typedef Grid::SpWilsonImplR FermionImplPolicy;
    typedef Grid::SpWilsonFermionD FermionAction;
    typedef typename FermionAction::FermionField FermionField;

    typedef Grid::GenericSpHMCRunnerHirep<TheRepresentations, Grid::MinimumNorm2> HMCWrapper;

    DJ<HMCWrapper> hmcdj(argc, argv);

    // Print the layout
    Grid::GridLogLayout();

    /* Action */
    Grid::RealD beta = 6.95;
    Grid::RealD mass = -0.87;

    Grid::SpWilsonGaugeActionR Waction(beta);

    auto GridPtr = hmcdj.TheHMC.Resources.GetCartesian();
    auto GridRBPtr = hmcdj.TheHMC.Resources.GetRBCartesian();

    Grid::SpFundamentalRepresentation::LatticeField U(GridPtr);

    FermionAction FermOp(U, *GridPtr, *GridRBPtr, mass);

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