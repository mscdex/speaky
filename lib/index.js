var ReadableStream = require('stream').Readable;
var inherits = require('util').inherits;

var Binding = require('../build/Release/speaky.node').Speaky;

module.exports = Speaky;

function Speaky(taPath, sgPath) {
  /*if (typeof taPath === 'string' && typeof sgPath === 'string')
    this.setVoice(taPath, sgPath);
  else
    this._handle = null;*/
  if (typeof taPath !== 'string')
    throw new Error('taPath argument must be a string');
  if (typeof sgPath !== 'string')
    throw new Error('sgPath argument must be a string');
  this._handle = new Binding();
  this._handle.setVoice(taPath, sgPath);
  this._queue = [];
  this._stream = null;
}

/*Speaky.prototype.setVoice = function(taPath, sgPath) {
  if (typeof taPath !== 'string')
    throw new Error('taPath argument must be a string');
  if (typeof sgPath !== 'string')
    throw new Error('sgPath argument must be a string');
  if (!this._handle) {
    this._handle = new Binding();
  }
  this._handle.setVoice(taPath, sgPath);
};*/

Speaky.prototype.speak = function(text) {
  if (!this._handle)
    throw new Error('Voice not set');
  if (typeof text !== 'string')
    throw new TypeError('text argument is not a string');
  var stream = new AudioStream(this._handle, text);
  var self = this;
  stream.on('end', function() {
    processQueue(self);
  });
  if (this._stream)
    this._queue.push(stream);
  else {
    stream._read = stream._realRead;
    this._stream = stream;
    var r = this._handle.speak(text);
    if (r === true)
      stream._noMoreText = true;
  }
  return stream;
};

function processQueue(speaky) {
  if (speaky._queue.length > 0) {
    var stream = speaky._queue.shift();
    speaky._stream = stream;
    stream._read = stream._realRead;
    var r = speaky._handle.speak(stream._text);
    // Text is only needed once, unset it here to allow easier GC
    stream._text = null;
    if (r === true)
      stream._noMoreText = true;
    // Kick off audio data stream
    stream._read();
  } else
    speaky._stream = null;
}

function AudioStream(handle, text) {
  ReadableStream.call(this);
  this._handle = handle;
  this._text = text;
  this._noMoreAudio = false;
  this._noMoreText = false;
}
inherits(AudioStream, ReadableStream);

AudioStream.prototype._read = function(n) {};
AudioStream.prototype._realRead = function(n) {
  var buffer = new Buffer(4096);
  var handle = this._handle;
  var nb;
  var r;

  if (this._noMoreAudio) {
    // Seed more text into the synthesizer to produce audio data
    r = handle.speak();
    if (r === true)
      this._noMoreText = true;
    this._noMoreAudio = false;
  }

  // getAudio() fills `buffer` as much as possible to avoid crossing the
  // JS<->C boundary too much
  nb = handle.getAudio(buffer);
  this._noMoreAudio = (nb < 0);
  if (this._noMoreAudio)
    nb *= -1;
  if (nb === buffer.length)
    r = this.push(buffer);
  else
    r = this.push(buffer.slice(0, nb));
  if (this._noMoreAudio && this._noMoreText)
    return this.push(null);
};
