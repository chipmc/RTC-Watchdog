# RTC-Watchdog

This project is part of an effort to build a programmable watchdog for the Particle devices.  This approach should be more flexible than the TPL5010

# Hardware 

I have laid out a sample board to support this project. You can follow this link to order the bare board from OSHPark
Link: https://oshpark.com/shared_projects/rNxANV7V

The Board is Open Source, here is the repo with the Bill of Materials - https://github.com/chipmc/DS1339-RTC

# Concept of Operations

This goal for this project is to provide a more flexible - programmable - watchdog timer for Particle devices.  Specifically, it will enable these use cases:

1) Tying the Interrupt pin to the Enalbe line on the new 3rd Gen Particle devices should reduce sleep power draw to almost nothing
2) Using the programmable nature of the timer to enable adaptive sampling when tied to a hardware interrupt pin
3) Use as an external watchdog timer when tied to the reset pin

Thank you to Sridhar Rajagopol for the original DSRTC library.

This work was sponsored by Jay Ham, Colorado State University - Thank You!
