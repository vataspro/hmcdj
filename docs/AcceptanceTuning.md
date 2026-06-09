# Acceptance Rate Tuning

## Introduction

Frequently when running simulations with the HMC the
tuning the Molecular Dynamics (MD) parameters are
non-trivial to tune.

In order to circumvent this issue `hmcdj` has a built-in
acceptance rate tuning functionality.

Built on the assumption that the user specifies the
trajectory length `trajL`, the code adapts the number
of molecular dynamics steps `MDsteps`.

Management of the acceptance rate takes place in three
discrete phases:

- Initialising
- Tuning
- Monitoring

## Initialisation phase

When the ensemble begins, a short period with no accept/reject
step takes place. Here the acceptance rate is not monitored.
The number of trajectories corresponding to this is given by the
`Thermalisation` in the track.

The acceptance rate is also not monitored for a short period after
the accept/reject step is activated. This is to further thermalise the
chain, using the provided `MDsteps` in the track as the initial guess.
The number of trajectories in this period is given by `num_initial_skips`.

## Tuning Phase

In the tuning phase, the HMC is run repeatedly for `num_tuning_samples`
trajectories. The acceptance of each step is saved and then used to estimate
the acceptance rate.

If the acceptance rate is not within `acceptance\_rate\_flexibility` of the
`target\_accepatance\_rate` then `MDsteps` will be tuned.

In order to find the optimal number of `MDsteps`, the
formula for the integration step

$$
\Delta\tau =
\frac{\mathrm{trajL}}{\mathrm{MDsteps}}
$$

as a function of the acceptance rate is used:

$$
P_{acc} = \mathrm{erfc}(
          \lambda \frac{(\Delta\tau)^2}{2}).
$$

Provided a target acceptance rate, `MDsteps` is
adjusted to obtain the desired step size.
This is implented by measuring the acceptance rate
and estimating the value of the constant $\lambda$.

If the target acceptace rate is reached, the process will run one
more time without changing the MDsteps, in order to verify that
the acceptance rate remains in the target range.

This process is run for `max_tuning_steps` and will exit with error
if the target acceptance rate is not reached by that time.

### File creation during tuning phase

As it may take multiple runs to tune the acceptance rate, or tuning
may be completed when restarting, `hmcdj` creates the `tuning\_status.xml` file
to track the progress of the tuning.

This file contains a flag on which phase of the tuning is active, the current
trajectory number and the acceptance value (traj, acc).

In cases of the previous run not terminating on the final trajectory,
the excess steps must be dropped from the file and recomputed.

## Monitoring Phase

After the tuning has been completed (or if the tuning is deactivated),
`hmcdj` will monitor the acceptance rate in real time, ensuring that it
does not leave the target range.
