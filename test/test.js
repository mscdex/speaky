var Speaky = require('../lib/index');

var assert = require('assert');

var speaky = new Speaky('/usr/share/pico/lang/en-US_ta.bin',
                        '/usr/share/pico/lang/en-US_lh0_sg.bin');

var streamData1 = [];
speaky.speak('node j s rules!')
      .on('data', function(data) {
        streamData1.push(data);
      })
      .on('end', function() {
        streamData1 = Buffer.concat(streamData1);
      });

var streamData2 = [];
speaky.speak('Help, I am trapped in a computer!')
      .on('data', function(data) {
        streamData2.push(data);
      })
      .on('end', function() {
        streamData2 = Buffer.concat(streamData2);
      });

process.on('exit', function() {
  assert(Buffer.isBuffer(streamData1));
  assert(streamData1.length > 0);
  assert(Buffer.isBuffer(streamData2));
  assert(streamData2.length > 0);
});
