# Filament playground!

A place to play with Google Filament.

## Filament instalation

Clone the Filament repository:

```
mkdir -p ~/workspace && cd ~/workspace
git clone https://github.com/google/filament.git
cd filament
git checkout v1.8.1
```

Install the Filament dependencies:

```
sudo apt-get install clang-7 libglu1-mesa-dev libc++-7-dev libc++abi-7-dev ninja-build libxi-dev
```

Compile Filament:

```
./build.sh release
```

Testing:

```
./out/cmake-release/samples/suzanne
```

![A first sample](images/suzane.png)

References:
  * [Building Filament](https://github.com/google/filament/blob/main/BUILDING.md)

## Creating a custom scene

As far as I see the only mesh formats supported are `obj` and `fbx`. These
formats are supported by a tool named `filamesh`, which seems to convert from
a few mesh formats to a binary representation used in Filament.

See [filamesh README.md](https://github.com/google/filament/tree/main/tools/filamesh). See also [this related issue](https://github.com/google/filament/issues/2634).


## Other resources

[External example](https://github.com/cgmb/hello-filament/blob/master/CMakeLists.txt) showing how to link your demo against Filament.