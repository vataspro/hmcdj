---
title: Building new hmcdj decks
---

The basic structure of an hmcdj deck
is similar to that of a Grid HMC program,
but with a lot of the boilerplate abstracted away by hmcdj.
The primary means to interact with hmcdj is via the `DJ` class,
which is templated on
the specific specialisation of `GenericHMCRunner` to be used.
Parameters to be exposed in the track are created as instances of `djParameter`,
templated on the data type of the parameter,
and passed to the `DJ` constructor wrapped in a `djParameterList`.

## Example: `PureGauge`

Let's look in detail at the `PureGauge` deck:

``` c++
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

  hmcdj.Play();

  return 0;
}
```

Working through this:

``` c++
#include <Grid/Grid.h>
#include <hmcdj/hmcdj.h>
```

Both Grid and hmcdj's headers are needed;
Grid's for the physics,
and hmcdj's for the hmcdj-specific functionality.

``` c++
int main(int argc, char* argv[]) {
```

We will need to pass `argc` and `argv` to the `DJ` later,
so we can't use `int main(void)`.

``` c++
  typedef Grid::GenericSpHMCRunner<Grid::MinimumNorm2> HMCWrapper;
```

This deck is for pure gauge $Sp(2N)$,
so we need `GenericSpHMCRunner`.

``` c++
  // Define parameters
  auto beta = djParameter<double>("beta");
```

The pure gauge action has a single parameter,
$\beta$,
which we represent as a floating-point number.
If we wanted an integer parameter instead,
we could use `djParameter<int>` to represent this.
The name `"beta"` is what will be used to refer to this parameter in the track.

``` c++
  djParameterList params = {beta};
```

In order to pass the parameter(s) to the `DJ`,
we must wrap them in a `djParameterList`.

``` c++
  DJ<HMCWrapper> hmcdj("PureGauge", argc, argv, params);
```

Now we instantiate the `DJ`.
We pass the `argc` and `argv`
so that hmcdj and Grid can process their respective command-line arguments,
and the `params` so hmcdj knows what parameters to look for in the track.

At this point in execution,
the track will be read in by hmcdj.

``` c++
  /* Observables */
  // Add the Plaquette observable
  typedef Grid::PlaquetteMod<HMCWrapper::ImplPolicy> PlaqObs;
  hmcdj.TheHMC.Resources.AddObservable<PlaqObs>();
  // Add the temporal Polyakov Loop observable
  typedef Grid::PolyakovMod<HMCWrapper::ImplPolicy> PolyakovObs;
  hmcdj.TheHMC.Resources.AddObservable<PolyakovObs>();
```

We can now add observables as we would in any Grid HMC application,
with the difference that hmcdj is managing the `GenericHMCRunner` instance,
so we need to use `hmcdj.TheHMC` to refer to it.

``` c++
  /* Action */
  Grid::SpWilsonGaugeActionR Waction(beta);

  Grid::ActionLevel<HMCWrapper::Field> Level1(1);
  Level1.push_back(&Waction);
  hmcdj.TheHMC.TheAction.push_back(Level1);
```

Similarly,
adding actions is the same as for Grid's HMC;
minimally,
an action instance and an `ActionLevel` instance are needed.
The former is `push_back`ed to the latter,
and then the latter `push_back`ed to the action,
accessed from `hmcdj.TheHMC`.

``` c++
  hmcdj.Play();
```

With all setup complete,
we can now play the track.
This will start a new run or resume from checkpoint,
and perform the requested Monte Carlo updates.

``` c++
  return 0;
}
```

hmcdj will exit with an error code in case of any errors,
so if control returns to `main()` then the run completed successfully.

## Deck principles

Things to bear in mind when writing a new deck:

### Parameters

You should only create `djParameter`s
for things that the user is likely to need to control
and vary from one ensemble to another.
This could be physics parameters like the fermion mass,
or algorithmic parameters like
the $b$ and $c$ coefficients in the Möbius Domain Wall Fermion action.

### Example track

After creating the deck,
create an example track in the `example_tracks` directory.
This should be used to test the deck;
do not work with or share a deck until the example track runs successfully.

### Naming things

If the deck is in the file `DeckName.cc`,
then the `DJ` should be initialised with the name `DeckName`,
and the example track should be called `DeckNameTrack.yaml`.

### Custom pathing

For the majority of decks,
the default directory tree used by hmcdj should be sufficient.
If you find that for some reason your deck needs an alternative pathing setup,
the `DJ` constructor accepts an additional argument of type `pathCallback`,
a function pointer accepting a pointer to an `EnsembleReeader` and a `djParameterList`,
and returning `std::filesystem::path`,
defining a custom path relative to [the hmcdj base path](running.md#output-directory).

### Feeding back

For reproducibility,
you should commit the deck and example track to the repository,
and only use the committed version in production workflows.

In particular,
members of the TELOS Collaboration
should pull request new decks to this repository
before publishing any work using them.
In accordance with the [TELOS Collaboration reproducibility guide][guide],
journal articles discussing configuration production
should reference the commit ID of hmcdj used for this.

[guide]: <https://github.com/telos-collaboration/strategy>
