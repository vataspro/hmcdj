---
title: "Running hmcdj"
---

## Configure a track

Before we can run hmcdj,
we need to select a deck,
and prepare a track for it to play.

For the sake of example,
let's say we wish to study an Sp(2N) pure gauge theory.
The `PureGauge` deck performs such studies.

For each deck,
there is an example/template track in the `example_tracks/` directory.
Let's take a copy of that into a working directory and edit it.
Assuming that you've just built the deck,
so starting in the `build/decks` directory,

``` shellsession
mkdir testrun_puregauge
cd testrun_puregauge
cp ../../../example_tracks/PureGaugeTrack.yaml track.yaml
nano track.yaml
```

(You are of course free to use your editor of choice.)

The `PureGauge` track looks as follows:

``` yaml
Global:
  name: "Ensemble"

  volume:
    nx: 4
    ny: 4
    nz: 4
    nt: 4


  checkpoint:
    saveInterval: 5
    format: "IEEE64BIG"

    configurations:
      prefix: "cfg_ckpoint"

    rng:
      prefix: "rng_ckpoint"

  HMC:
    MD:
      MDsteps: 10
      trajL: 1.0

    Thermalisations: 0

    Trajectories: 10

    StartingType: "HotStart"

  HMCDJ:
    EnsembleDirectory: "."

PureGauge:
    beta: 6.9
```

The `Global` block is common to all hmcdj decks.
The most important aspects of this:

- `volume` sets the lattice volume.
  (This is _instead_ of the `--grid` command-line option
  you may be familiar with if you have used Grid before.)
- `trajL` sets the length of the HMC trajectory.
- `Thermalisations` sets the number of steps made without a Metropolis check
  at the start of the Markov chain,
  to allow the system to reach close enough to equilibrium
  that regular updates have a reasonable acceptance.
- `Trajectories` sets the target number of trajectories to generate.
- `StartingType` may be `HotStart`, `ColdStart`, or `TepidStart`;
  this sets the initial entropy of the lattice at the start of the Markov chain.

In addition,
each track has a second block named after the deck it is designed for.
In the case of `PureGauge`,
the `PureGauge` block has a single parameter,
`beta`,
the inverse gauge coupling.

Let's use a larger lattice volume and a finer coupling,
add some steps without a Metropolis check to the beginning,
and generate a larger number of trajectories:

``` yaml
Global:
  name: "Ensemble"

  volume:
    nx: 16
    ny: 16
    nz: 16
    nt: 32


  checkpoint:
    saveInterval: 5
    format: "IEEE64BIG"

    configurations:
      prefix: "cfg_ckpoint"

    rng:
      prefix: "rng_ckpoint"

  HMC:
    MD:
      MDsteps: 10
      trajL: 1.0

    Thermalisations: 10

    Trajectories: 100

    StartingType: "HotStart"

  HMCDJ:
    EnsembleDirectory: "."

PureGauge:
    beta: 7.2
```

## Running a deck

One you have prepared a track and compiled the deck,
we are ready to start a run:

``` shellsession
../PureGauge track.yaml
```

This will run on a single CPU or GPU.
Any additional options will be passed through to Grid
(see below);
these must be placed after the track filename.

### Running with MPI

If we have compiled with MPI,
we can run in parallel:

``` shellsession
mpirun -n 2 ../PureGauge track.yaml --mpi 1.1.1.2
```

If you are using Slurm,
then `srun` can also be used here,
and the `-n 2` omitted,
depending on your cluster's setup.

The `--mpi` argument here is required,
and is passed through directly to Grid;
this controls the decomposition of the global 4-volume between MPI processes.
The product of the four values must equal the number of MPI ranks.

Grid imposes some constraints on the lattice geometry,
particularly with MPI:

- When using red-black (even-odd) preconditioning,
  each local dimension
  (the global lattice extent divided by the number of processes in that direction)
  must have a power of two in its prime factors.
- Grid additionally vectorises across the last two dimensions
  (z and t);
  as such,
  the last two local dimensions must have an additional factor of two for vectorisation.
  This means that one spatial and the temporal dimension
  must typically be multiples of 4.

### Output

All hmcdj output is placed in a common directory structure,
by default based in `${HOME}/hmcdj_ensembles`.
Within this base directory,
the subdirectory location depends on the deck and track parameters.
For example,
the output of the `PureGauge` deck using the track `example_tracks/PureGaugeTrack.yaml`
creates the subdirectory `PureGauge/Nc4/beta6.9/4.4.4.4/HotStart`.

Within this subdirectory,
hmcdj generates three kinds of output file:

- Field configuration checkpoints, placed in `cnfg`
- Random number generator state checkpoints, placed in `rand`
- Log files, placed in `logs`, and duplicated on standard output

The base directory can be overriden;
see the next section.

## Useful run-time options

### Output directory

When multiple collaborators are working on a project,
it is useful for jobs to use a common working directory
rather than each collaborator having a separate `hmcdj_ensembles` tree.

To set a base location for hmcdj,
export the environment variable `HMCDJ_BASE_PATH`.
For example,
on Tursa,
one might run:

```bash
export HMCDJ_BASE_PATH=/mnt/lustre/tursafs1/home/dp208/dp208/shared
```

Individual decks can also be set up to override this standard directory structure.
For details,
see [Building new hmcdj decks](newdecks.md).

### Log control

Grid accepts a `--log` option to control the verbosity of logging.
Useful options here are:

- `NoIntegrator`:
  Significantly reduce the size of log files
  by omitting information about the integration.
- `Colours`:
  Colour parts of the log by the type of message present on the line.
  Useful for debugging,
  but should be disabled in production.
- `Debug`:
  Output additional information for debugging purposes.

The full list of options is [on Grid's GitHub][log-options];
other options are likely only useful for more detailed debugging.

### Timing

hmcdj will integrate with Slurm
to identify the available time in a job,
and avoid starting a trajectory
if there will not be sufficient time to complete it.

This can be used in conjunction with the `--time-min` option to `sbatch`
to allow Slurm to opportunistically schedule a job in a shorter time slot than ideal,
while still allowing longer time limits where necessary.

On clusters with job preemption,
this also interacts with the `--signal` option;
this should be configured to send signal `SIGUSR1`,
with longer notice than the expected time to complete a trajectory.
For example,
if each trajectory takes around 28 minutes to complete,
then the Slurm directive

``` bash
#SBATCH --signal=R:USR1@1800
```

would give 30 minutes' notice,
sufficient to complete the current trajectory and cleanly exit.

[log-options]: <https://github.com/telos-collaboration/Grid/blob/develop/Grid/log/Log.cc#L91-L104>
