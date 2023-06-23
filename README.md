# Scenic.Driver.Debug

This is the main "debug" renderer for Scenic applications.

It replaces the nanovg backend with just an absurd amount of printfs.

**This is currently just gross code, don't be surprised to find sharp edges.**

## Installation

If [available in Hex](https://hex.pm/docs/publish), the package can be installed
by adding `scenic_driver_debug` to your list of dependencies in `mix.exs`:

```elixir
def deps do
  [
    {:scenic_driver_debug, "~> 0.11.0"}
  ]
end
```

Honestly, you'll probably need to manually load this from a local checkout until its up on hex.

## Configuration

Example:

```elixir
[
  module: Scenic.Driver.Debug,
  window: [title: "Local Window", resizeable: true]
]
```

There are quite a few new options as well. It uses `NimbleOptions` to confirm them, so look at the `Scenic.Driver.Local` module for details.


## Prerequisites

This driver requires Scenic v0.11 or up.

### Installing on MacOS

The easiest way to install on MacOS is to use Homebrew. Just run the following in a terminal:

```bash
brew update
brew install pkg-config
```


Once these components have been installed, you should be able to build the `scenic_driver_debug` driver.

### Installing on Ubuntu 18.04

The easiest way to install on Ubuntu is to use apt-get. Just run the following:

```bash
apt-get update
apt-get install pkgconf
```

Once these components have been installed, you should be able to build the `scenic_driver_debug` driver.

### Installing on Ubuntu 20.04

The easiest way to install on Ubuntu is to use apt-get. Just run the following:

```bash
apt-get update
apt-get install pkgconf 
```

Once these components have been installed, you should be able to build the `scenic_driver_debug` driver.

### Installing on Windows

First, make sure to have installed Visual Studio with its "Desktop development with C++" package.

Once these components have been installed, you should be able to build the `scenic_driver_debug` driver.

## Documentation

Not there yet.

