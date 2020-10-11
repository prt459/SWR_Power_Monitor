# SWR_Power_Monitor
A simple SWR and power monitor, intended for an Arduino Nano and directional coupler built into an AM transmitter.  

V1.0, 11 Oct 2020 - first version.

This script was coded to use the directional coupler described in:
'A PIC16F876 based, automatic 1.8 – 60 MHz SWR/WATTmeter' a project from “Il Club Autocostruttori” 
of the Padova ARI club' by IW3EGT and IK3OIL, see:  
http://www.ik3oil.it/_private/Articolo_SWR_eng.pdf   

No attempt was made to write code to accurately determine RF power across a wide range of SWRs and frequencies. 
Instead, a set of power readings from the station transmitter and the correspondng analogRead values (0..1024)
was mapped into an approximation using the curve fitting calculator at https://www.dcode.fr/equation-finder 

No claim to either SWR or power accuracy is made; this is an indicative tool, not an accurate one.   

The main purpose was to provide a digital readout of approximate power and SWR, and to implement 
a 'High SWR' interlock to reduce power in the presence of high SWR.  The time taken to detect and raise this 
interlock is dependent on several factors including the processing speed of the Nano; it is in the 
10s of milliseconds range; this may not suit or even protect your transmitter!  

All constants are #define'd.  
The displays dim after a configurable period of inactivity. 
The High SWR interlock resets when the SWR drops below the threshold. 

Comments, feedback, improvements, stories welcome. 

Paul VK3HN.  https://vk3hn.wordpress.com/  
