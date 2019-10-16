# Arduino project GSM time relay.
GSM time relay is able to turn on and off the power of the payload on a call from a mobile phone and via SMS message. Switching on and off is also carried out at a certain time, the setting of which is via SMS.
In addition, it is possible to obtain the current relay settings, control the reboot, set access from various phone numbers.
## Set relay by GSM_relay_set.ino
For setting relay need to upload sketch GSM_relay_set.ino.
There need to set numbers of on/off positions, time on/off, and phone numbers for control.
After connect to can set time in console by send value of time, date, mounth and year in format "day.month.year hour:minute:sec".
Other relay setting to can set after upload sketch GSM_relay.ino by sms-messages.
