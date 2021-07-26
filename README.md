# cry-with-openmc

The "Cosmic-ray Shower Library" (CRY) provides the functionality to generate secondary particles from cosmic rays as source particles. The library is provided by Lawrence Livermore National Laboratory (https://nuclear.llnl.gov/simulation/).

cry-with-openmc provides code for a shared library that can be directly as a source in recent versions of OpenMC, as described in https://docs.openmc.org/en/stable/usersguide/settings.html#custom-sources. 

## Installation

On a Linux system with gcc and cmake, and OpenMC's libraries/CMake settings installed in `/usr/local/lib/`, the following steps should suffice to build the library.
* Clone the repository, enter the cloned folder
* `mkdir build; cd build`
* `cmake ../`
* `make`


## Usage

To use the produced shared library, you need to set the `openmc.source.library` variable (or xml setting) to the path of the produced `libsource.so`. It is also possible to have the `libsource.so` locally in the folder for the calculations. In that case the variable should be set to `./libsource.so`. In addition, the library needs access to the contents of the `data` folder of the CRY source folder. This folder needs to be at the local calculation folder (symbolic link is sufficient).

It is further necessary to set the following parameters via `openmc.source.parameters` (or xml setting). The parameters need to be provided as a one-line string, with the following order:

`<max neutron energy in eV> <discard/limit> <xoffset> <yoffset> <zoffset> <CRY settings>`

Neutrons that are sampled with an energy higher than `<max neutron energy in eV>` are either discarded (and resampled), or set to the maximum energy (minus 1meV to ensure absolute less). This allows to run the source with finite energy cross section libraries (most commonly used have a limit of 20MeV). The offsets `<x/y/zoffset>` can be used to adjust the position and height of the source. The source is always a square, with the size given in the CRY settings. Adjusting `<zoffset>` does not change the altitude setting for CRY.

`<CRY settings>` can be set according to the CRY manual. It is advised to turn all particles off except for neutrons (otherwise OpenMC will start neutrons with, e.g., muon energies). An example for these settings is `returnNeutrons 1 returnProtons 0 returnGammas 0 returnMuons 0 returnElectrons 0 returnPions 0 date 1-1-2008 latitude 50.173833 altitude 0 subboxLength 10.000000`.

Then openmc can be executed (*single core only*).

## Limitations

As pointed out above, the library currently allows only for *single core simulations*. Multi-threading produces a segmentation fault. This is probably due to the way the OpenMC random number generator function is passed to CRY. As the OpenMC RNG always has an argument, directly passing a pointer to the function definition was insufficient. Now, a wrapper function is passed to a slightly modified CRY source, and the wrapper function has access to another wrapper in the source class, which handles the `seed` argument. Better solutions / suggestions are welcome!

OpenMC sources typically start one particle per event. The CRY class partly produces multiple, correlated neutrons. In case that happens, the resulting neutrons are pushed back to the next event. For example if event 157 would be one with 3 neutrons, they are started as event 157, 158 and 159 in OpenMC.

## JENDL 4.0 High Energy Cross Sections

A significant fraction of cosmic ray neutrons has an energy greater than 20 MeV. The Japan Atomic Energy Agency, as part of its JENDL neutron cross section library series provides a higher energy version of JENDL 4.0 for a selected number of nuclides: https://wwwndc.jaea.go.jp/ftpnd/jendl/jendl40he.html. Cross sections are given in ace format, thus can be converted easily for the use in OpenMC.

The script `create-jendl40he-library.py` can be used to automatically download, unpack and convert the library. It requires that the script "openmc-ace-to-hdf5" is accessible in the `$PATH` variable. There will be some errors/warnings towards the end, because the proton library is not downloaded, buy included in the xsdir file. These warnings can be ignored. The resulting OpenMC library is stored in the folder `jendl40he` and can be used for simulations with neutron energies up to 200 MeV.