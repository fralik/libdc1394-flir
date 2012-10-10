This a clone of official libdc1394 repo that adds support for FLIR cameras.

The patch itself is in file *libdc1394-2.1.2-flir.patch* . Unfortunately, original 
supplement documentation is gone. IIDC specs says that camera should store
several predefined values in some registers. Libdc1394 reads the values from
these registers and based on the results decides whether it can handle the camera.

The patch adds a value that is stored in registers of FLIR camera. It also adds
appropriate checks for this value.

The license complies with that of original libdc1394 library. The 1394-Based 
Digital Camera Control Library is licensed under the Lesser
General Public License (short LGPL, see file COPYING in the source
distribution). Other files in the source archives not belonging to but being
part of the build procedure of libraw1394 are under their own licenses, as
stated at the top of the individual files.

Those of you who read Russian - please refer to general [overview](http://habrahabr.ru/post/86230/)
of the patch.