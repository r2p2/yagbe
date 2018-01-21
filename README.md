# YAGBE

## DEPENDENCIES

* SDL2 for graphics

## BUILD

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## EXECUTE

```
./yagbe <PATH_TO_ROM>
```

## ISSUES

* no sound implemented
* timing issues; don't have the cycle/timer/vblank relations figured out
* mcb implementation lacking
* only Tetris seems to work (well, it does something)
* code and architecture are a mess
* high cpu usage
