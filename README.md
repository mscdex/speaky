
Description
===========

A binding to the SVOX Pico engine (libttspico) for [node.js](http://nodejs.org/) for performing text-to-speech.

[![Build Status](https://travis-ci.org/mscdex/speaky.svg?branch=master)](https://travis-ci.org/mscdex/speaky)


Requirements
============

* [node.js](http://nodejs.org/) -- v0.10.0 or newer
* libttspico* packages -- Tested on Ubuntu with version 1.0+git20130326-3


Install
=======

    npm install speaky


Example
=======

```javascript
var fs = require('fs');

var Speaky = require('speaky');

// Use the en-US voice that comes with the libttspico-data package.
// The order of the 'ta' and 'sg' files does not matter, as long as both types
// are passed to the constructor.
// 'ta' === 'text analysis'
// 'sg' === 'signal generation'
// The path to these languages files may vary depending on your OS distro.
var speaky = new Speaky('/usr/share/pico/lang/en-US_ta.bin',
                        '/usr/share/pico/lang/en-US_lh0_sg.bin');

speaky.speak('node j s rules!')
      .pipe(fs.createWriteStream('out.pcm'));
speaky.speak('Help, I am trapped in a computer!')
      .pipe(fs.createWriteStream('out2.pcm'));

// Audio data is formatted as raw 16KHz, 16-bit signed integer PCM.
// The audio can be played via sox with:
//   play -t raw -r 16000 -b 16 -c 1 -e signed-integer out.pcm
```


API
===

Speaky methods
--------------

* **(constructor)**(< _String_ >taPath, < _String_ >sgPath) - Creates and returns a new Speaky instance that is set to use the voice described by the voice files at `taPath` and `sgPath`.

* **speak**(< _String_ >text) - _ReadableStream_ - Enqueues `text` to be synthesized to raw, 16KHz, 16-bit signed integer PCM audio samples. The Readable stream returned will receive this audio data as it is generated.
