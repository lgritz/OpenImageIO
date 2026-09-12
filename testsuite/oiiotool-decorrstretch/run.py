#!/usr/bin/env python

# Copyright Contributors to the OpenImageIO project.
# SPDX-License-Identifier: Apache-2.0
# https://github.com/AcademySoftwareFoundation/OpenImageIO


redirect = " >> out.txt 2>&1 "


# Synthesize an image that hides two features in plain sight: a hazy
# gradient, whose three channels are therefore almost perfectly correlated,
# plus two lines of text that depart from that haze by only a few thousandths,
# along two different color directions. Neither line is visible to the eye,
# nor even a full value of an 8 bit file. A decorrelation stretch amplifies
# the color directions that carry them until they are impossible to miss.

hazy = ("--pattern fill:top=0.20,0.30,0.36:bottom=0.80,0.65,0.52 320x240 3 "
        # Grain, the same in every channel, so it stays on the bright axis
        # that the stretch leaves alone.
        "--pattern noise:type=gaussian:mean=0:stddev=0.02:mono=1:seed=1 320x240 3 "
        "--add "
        # "HIDDEN": +R -G, 0.002 either way.
        "--pattern constant:color=0,0,0 320x240 3 "
        "--text:x=160:y=80:xalign=center:size=52:font=DroidSerif:color=1,-1,0 "
        "HIDDEN --mulc 0.002 --add "
        # "IN PLAIN SIGHT": +R +G -B, on a different color axis again.
        "--pattern constant:color=0,0,0 320x240 3 "
        "--text:x=160:y=160:xalign=center:size=30:font=DroidSerif:color=1,1,-2 "
        "\"IN PLAIN SIGHT\" --mulc 0.004 --add ")

command += oiiotool (hazy + "-d half -o hazy.exr")

# What the eye (and an 8 bit file) can make of it unaided: nothing.
command += oiiotool ("hazy.exr -d uint8 -o decorr-original.tif")

# The stretch itself. Both lines of text appear, in two different colors,
# because they were hidden along two different color axes.
command += oiiotool ("hazy.exr --decorrstretch -d uint8 -o decorr-default.tif")

# scale exaggerates the contrast of the result beyond the original's.
command += oiiotool ("hazy.exr --decorrstretch:scale=2 -d uint8 -o decorr-scale2.tif")

# percentile fills the display range instead of keeping the original spread,
# saturating 1% of the pixels at each end.
command += oiiotool ("hazy.exr --decorrstretch:percentile=1 -d uint8 -o decorr-percentile.tif")

# sigma and mean flatten every channel to the same spread and center.
command += oiiotool ("hazy.exr --decorrstretch:sigma=0.2:mean=0.5 -d uint8 -o decorr-sigma.tif")

# correlation mode gives the weakly varying blue channel the same say as the
# strongly varying red one, which lands the background in a different place.
command += oiiotool ("hazy.exr --decorrstretch:mode=correlation -d uint8 -o decorr-correlation.tif")
command += oiiotool ("hazy.exr --decorrstretch:mode=correlation:percentile=1 "
                     "-d uint8 -o decorr-correlation-percentile.tif")

# Only the channels asked for are touched. Decorrelating green and blue alone
# leaves red as it was, so "HIDDEN", which is hidden in red against green,
# stays hidden, and "IN PLAIN SIGHT", which spans all three channels, is only
# faintly recovered.
command += oiiotool ("hazy.exr --decorrstretch:firstchannel=1:nchannels=2 "
                     "-d uint8 -o decorr-greenblue.tif")

# Errors
command += oiiotool ("hazy.exr --decorrstretch:percentile=60 -o out.tif",
                     failureok = True)
command += oiiotool ("hazy.exr --decorrstretch:mode=bogus -o out.tif",
                     failureok = True)


# Outputs to check against references
outputs = [
            "hazy.exr",
            "decorr-original.tif",
            "decorr-default.tif",
            "decorr-scale2.tif",
            "decorr-percentile.tif",
            "decorr-sigma.tif",
            "decorr-correlation.tif",
            "decorr-correlation-percentile.tif",
            "decorr-greenblue.tif",
            "out.txt"
    ]
