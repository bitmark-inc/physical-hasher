# install libraries

~~~
sudo cp -p libcyusb* /usr/local/lib/
~~~


# setup of run-test script

~~~
cp -p run-test.config.sample run-test.config
~~~

Ensure the video device is correct: the default is `/dev/video1`
since it is likely that any built-in webcam is `/dev/video0`.


# running the script

To use default frame count as set by configuration file, run the program as:

~~~
./run-test
~~~

To set a different frame count use:

~~~
./run-test.sh 600
~~~


# note on the firmware

Currently the operation is to locate the zero point of the stepper,
which can take up to 100 steps.  Since the stepper is steps at frame
rate this can be up to 100 frames.

Once zero is established the stepper will ramp 0→60→0 in a repeating
pattern, this moves the lens a total of 0.8mm.


## data format

The binary dump is simply little endian 12 bit bayer data in the order:
~~~
GRGRGR...
bgbgbg...
GRGRGR...
bgbgbg...
~~~~

where:
* G = green pixel in red row
* R = red pixel
* g = green pixel in blue row
* b = blue pixel

so there are 1920 * 1080 * 2 bytes total per frame

# embedded data

If the pixel has a 12 bit value of `HML`  the bytes will be: `ML` `0H`

Since the "high-nibble" of even bytes is always zero some of these are
used to log some data from the processor, the high nibbles of pixels
at these offsets are used:

~~~
8192  focus low
8193  focus high
8194  reserved 1
8195  reserved 2
8196  reserved 3
8197  reserved 4
8198  reserved 5
8199  reserved 6
8200  flag: 0xa => data present,  0x0 => no data
~~~

The reserved are for logging a 24 bit values from auto focus, but this
is not currently enables so all will be zero.

The flag can be tested as:
~~~
if (0xa000 == (pixel[8200] & 0xf000)) {
  // extract the embedded data here (for example code see create-png.c)
}
~~~

Any processing of the pixel data should mask: `pixel[i] & 0x0fff` to
avoid interference from embedded data.
