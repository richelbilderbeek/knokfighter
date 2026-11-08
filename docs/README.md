# KnokFighter

KnokFighter is a Double Dragon clone with photorealistic graphics
and one of
[Richèl Bilderbeek's games](https://richelbilderbeek.github.io/games),
which he programmed together with Joost van den Bogaart.

- [Storyline (pdf)](KnokFighterStory_1_0.pdf)
- [Storyline (ppt)](KnokFighterStory_1_0.ppt)

![Start](start.png)

![GameKnokfighter_1_0.png](GameKnokfighter_1_0.png)

![GameKnokfighter_2_0.png](GameKnokfighter_2_0.png)

## Building the game

The game was originally written in C++Builder 6 using the VCL
(`ProjectKnokfighter.bpr`).
There is also a Qt version (`knokfighter.pro`), which is built
by continuous integration:

- [Build on Linux](.github/workflows/build_linux.yaml)
  checks that the game builds on Linux
- [Build on Windows](.github/workflows/build_windows.yaml)
  creates a zip file with the Windows executable,
  which can be downloaded from the workflow run

To build the Qt version locally:

```bash
mkdir build
cd build
qmake ../knokfighter.pro
make
```

