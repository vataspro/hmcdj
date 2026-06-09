---
title: "Introducing hmcdj"
---

hmcdj is a wrapper tool
for running configuration generation campaigns using the Grid library.
Its main goals are the following:

- Things that need to be changed across a study of a particular theory
  (for example,
  lattice volume,
  lattice coupling,
  fermion mass,
  etc.)
  should be easy to change.
- Things that should not be changed across a study of a particular theory
  (inverters,
  fermion content,
  etc.)
  should be harder to change.
- Things that can be automated
  (verifying that the acceptance is an appropriate value,
  resuming from the most recent checkpoint,
  writing the necessary metadata to share configurations via ILDG)
  should be automated.

It does this by introducing the following concepts:

- A "deck" is a program built on top of the core hmcdj library,
  to run the HMC for one specific theory.
  Decks are written in C++.
- A "track" is a file containing parameters,
  written in [YAML][yaml].
  A track is "played" by a deck.
  Each track is specific to its associated deck;
  it can't be played by a different deck.

[yaml]: <https://yaml.org>
