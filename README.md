# bb_pru_parallax_sonar
Sample Code for Beaglebone Black for Parallax Ping))) processing in PRU

This is a simple example for hard real time processing of Parallax Ping)))
on BeagleBone Black Rev C. 

Hard real time is required since we need to measure the response 
signal precisely in order to be able to calculate the distance measured:

http://www.parallax.com/product/28015

Luckily BeagleBone Black am3359 CPU provides two programmable real time
unit subsystems (PRU) that are specifically designed for this.

http://processors.wiki.ti.com/index.php/Programmable_Realtime_Unit_Subsystem

For device tree blob (dtb), copy the dts file over to BeagleBone Black and
run the device tree compiler:

cd /lib/firmware
dtc -b 0 -@ -I dts -O dtb -o ./BB-BONE-PRU-00A0.dtbo ./BB-BONE-PRU-00A0.dtbo.dts

The sample C++ code is part of a not yet complete robot hobby project. But
it could still be helpful for anyone who wants to tackle the same problem.

WARNING: This code is not throughly tested and it is provided as is. Use it
at your own risk.

For rationale, see my blog post:

http://tolgaceylan.tumblr.com/post/108720255534/first-experiences-with-beaglebone-black-hard-real