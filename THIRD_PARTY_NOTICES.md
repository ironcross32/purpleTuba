# Third-party notices

Purple Tuba is licensed under the GNU Affero General Public License v3 (see `LICENSE`).
It includes or is derived from the following third-party work.

## Pink Trombone (DSP algorithm)

The glottis, vocal tract and noise models in `src/dsp/` are a C++ port of
Pink Trombone, version 1.1 (March 2017), https://dood.al/pinktrombone/

```
Copyright 2017 Neil Thapen

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
```

## Simplex noise

`src/dsp/Noise.cpp` contains a port of the 2D simplex noise used by Pink Trombone
(based on example code by Stefan Gustavson, optimisations by Peter Eastman,
JavaScript conversion by Joseph Gentle). Placed in the public domain by its
original author.

## Libraries (git submodules under `libs/`)

| Library | License |
|---|---|
| JUCE (`libs/JUCE`) | AGPLv3 / commercial JUCE licence |
| clap-juce-extensions (`libs/clap-juce-extensions`), CLAP, clap-helpers | MIT |
| Catch2 (`libs/Catch2`, tests only) | Boost Software License 1.0 |
