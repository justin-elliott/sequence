# jell::sequence

An implementation of the ["A Direction for Vector"](http://wg21.link/p3147) proposal.

## Building

Building the project requires clang and libc++ version 20 or later.

```sh
mkdir build
cd build
cmake ..
make
```

## Testing

```sh
cd build
make && make test
```

## Code Coverage

```sh
cd build
make && make test && make coverage
```