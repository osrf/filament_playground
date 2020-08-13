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

References:
  * [Building Filament](https://github.com/google/filament/blob/main/BUILDING.md)

## Creating a custom scene

As far as I see they only mesh formats supported are `obj` and `fbx`.
