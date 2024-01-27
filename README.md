# teensy-audio-x86-stubs
[![audio-x64](https://github.com/newdigate/teensy-audio-x86-stubs/actions/workflows/audio-x64.yml/badge.svg)](https://github.com/newdigate/teensy-audio-x86-stubs/actions/workflows/audio-x64.yml)
[![audio-soundio](https://github.com/newdigate/teensy-audio-x86-stubs/actions/workflows/soundio.yml/badge.svg)](https://github.com/newdigate/teensy-audio-x86-stubs/actions/workflows/soundio.yml)

Teensy audio library ported to linux, with input and output via [libsoundio](https://github.com/andrewrk/libsoundio) highly experimental work-in-progress. 

Dependencies
* [teensy-x86-stubs](https://github.com/newdigate/teensy-x86-stubs)
* [teensy-x86-sd-stubs](https://github.com/newdigate/teensy-x86-sd-stubs)

Examples

* [analyze](extras/soundio/examples/analyze) 
  * analyze input and print peak / rms    
* [basic](extras/soundio/examples/basic)
  * output different frequency in left and right channels  
* [input](extras/soundio/examples/input)
  * connect sound in to sound out (careful, can cause feedback, use headphones) 
* [mixer](extras/soundio/examples/mixer)
  * mix an in-memory sample and a sine wave
* [playmemory](extras/soundio/examples/playmemory)
  * play an in-memory sample
* [queue](extras/soundio/examples/queue)
  * use a queue to record audio buffers
* [recordsine](extras/soundio/examples/recordsine)
  * record sine and save to file system  

# Soundio dynamic library loading issue when using XCode 15
* symptoms: compiles success, but when running, an error message appears about a missing symbol. A signal is thrown before the application runs during the dynamic loading of soundio.dylib
  ```
  dyld[3457]: Symbol not found: _soundio_backend_name
  Referenced from: <852044B9-0C60-374A-88FF-78C26EBA525E> /Users/goofy/code/
  Expected in:     <no uuid> unknown
  ```
* fix:
  * solution found in go-lang (https://forum.golangbridge.org/t/go-test-with-cgo-on-macos-and-dyld-library-path/32274/2)
  * noted here [my gist](https://gist.github.com/newdigate/8418a30039b9c1c849a1a20db2de81dc) 
```sh
install_name_tool -id /usr/local/lib/libsoundio.dylib /usr/local/lib/libsoundio.dylib
```

Credits
* taken from [PaulStoffregen/Audio](https://github.com/PaulStoffregen/Audio)
