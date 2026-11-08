# Knokfighter

- [Go to the nicely rendered website](https://richelbilderbeek.github.io/knokfighter/).

The game is built by continuous integration into the folder `docs/game`,
which is not part of this repository.

## Building the game

The game was originally written in C++Builder 6 using the VCL
(`ProjectKnokfighter.bpr`).
There is also a Qt version (`knokfighter.pro`), which is built
by the [Build game](.github/workflows/build_game.yaml) workflow.
On Windows, that workflow creates a zip file with the executable,
which can be downloaded from the workflow run.

To build the Qt version locally:

```bash
mkdir build
cd build
qmake ../knokfighter.pro
make
```

## Files used by continuous integration scripts

<!-- markdownlint-disable MD013 --><!-- Tables cannot be split up over lines, hence will break 80 characters per line -->

Filename                                  |Descriptions
------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------
[mlc_config.json](mlc_config.json)        |Configuration of the link checker, use `markdown-link-check --config mlc_config.json --quiet docs/**/*.md` to do link checking locally
[.spellcheck.yml](.spellcheck.yml)        |Configuration of the spell checker, use `pyspelling -c .spellcheck.yml` to do spellcheck locally
[.wordlist.txt](.wordlist.txt)            |Whitelisted words for the spell checker, use `pyspelling -c .spellcheck.yml` to do spellcheck locally
[.markdownlint.jsonc](.markdownlint.jsonc)|Configuration of the markdown linter, use `markdownlint "**/*.md"` to do markdown linting locally. The name of this file is a default name.
[.markdownlintignore](.markdownlintignore)|Files ignored by the markdown linter, use `markdownlint "**/*.md"` to do markdown linting locally. The name of this file is a default name.

<!-- markdownlint-enable MD013 -->
