# Contributing to hmcdj

hmcdj welcomes new contributions,
particularly those that align with the objectives of the project
and of the TELOS Collaboration:

- Enabling more easily reproducible research
- Reducing the possibility to introduce uncontrollable or unauditable errors in work
- Facilitating easier open sharing of data,
  following the [FAIR Principles][fair]

## High-level workflow

The repository follows a standard GitHub-based workflow:

1. Review the hmcdj [Code of Conduct][coc]
2. Fork this repository under your own username
3. Clone your fork to your development machine
4. Install `pre-commit` in your clone,
   so that code style checks are performed automatically:

       pre-commit install

5. Develop in a branch in your repository,
   following the guidelines below.
6. When you have finished development,
   open a pull request,
   filling out the pull request template.
7. Another hmcdj developer will review the request,,
   and may request a small number of changes,
   either to make the implementation align better with the planned direction of development,
   or to correct a possible oversight.
8. Make any requested changes,
   push them to your branch,
   and add a reply to the pull request once all requested changes have been made.
9. Steps 7 and 8 may repeat a couple of times until development converges.
10. Once agreement is reached,
    an hmcdj developer will merge the pull request.

## Developing and preparing a pull request

When working on hmcdj,
bear the following in mind:

1. All new features must come with an associated test,
   which is run automatically as part of the test suite.
2. All new features must be documented in the documentation.
3. All features and decks must build against the current `develop` branch of
   [the TELOS Collaboration fork of Grid][grid-telos].
   If a new feature is required,
   this must be pull requested to Grid-TELOS.
   The PR to hmcdj will not be merged until the prerequisite Grid PR has been.
4. The full test suite must be run in advance of opening the pull request.
5. `pre-commit` must pass cleanly on the change set.
   (If `pre-commit` was installed before committing,
   and has not been bypassed at commit time,
   this requirement will be met automatically.)

## Generative and agentic AI contributions

All pull requests to hmcdj must be authored by a "natural person" (you),
who takes responsibility for its content.
You must understand each line of code in the pull request.
While use of tools to improve the consistency and ease of programming is welcome
(as linters, autoformatters, static analysers, and similar have been for many years),
such tools must not be allowed
to develop, commit, or submit pull requests autonomously.
We additionally ask that you engage directly in the code review process.

Pull requests
that are clearly written by an autonomous mechanical "agent"
without review by a natural person
are not welcome,
and will be rejected without further review.
Similarly,
responses to code review that are obviously copy-pasted from a chatbot
are not welcome.
Repeated submission of such contributions will result in
you being blocked from the repository,
in accordance with the [Code of Conduct][coc].

[coc]: <CODE_OF_CONDUCT.md>
[fair]: <https://www.go-fair.org/fair-principles/>
[grid-telos]: <https://github.com/telos-collaboration/Grid>
