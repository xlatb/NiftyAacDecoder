# NiftyAacDecoder

A small decoder for MPEG-2 AAC (Advanced Audio Coding) audio. It supports
the LC (low complexity) profile.

## What is supported?

The only supported container format is ADTS. Files in this format normally
have a `.aac` extension.

If you have some AAC audio in a different container format, you can use
[ffmpeg](https://www.ffmpeg.org/) to convert it to ADTS without re-encoding
the audio:

`$ ffmpeg -i my-audio-file.m4a -acodec copy -vn my-audio-file.aac`

If you have some non-AAC audio, you'll need to re-encode it to AAC, which will
result in a bit of quality loss:

`$ ffmpeg -i my-audio-file.mp3 -profile:a mpeg2_aac_low my-audio-file.aac`

## How do I run it?

Run aac-to-wav, supplying a command-line argument with the filename of the
file to decode:

`$ ./aac-to-wav my-audio-file.aac`

The decoded audio will be written to the file `out.wav`.

## What about patents?

I am not a lawyer, but AAC-LC was first specified in MPEG-2 part 7 from 1997.

Wikipedia [notes](https://en.wikipedia.org/wiki/MPEG-2#Patent_pool) the
following regarding MPEG-2 patents:
> As of January 3, 2024, MPEG-2 patents have expired worldwide, with the
> exception of only Malaysia, where the last patent is expected to expire in
> 2035. The last US patent expired on February 23, 2018.

No doubt there are more advanced features that build on top of the original
AAC that are still covered by patents.
