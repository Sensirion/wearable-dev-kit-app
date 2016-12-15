# SensiSmart Pebble App

SensiSmart is Sensirion's demonstration pebble app to the Sensirion Smartstrap
(Backpack) hardware. The app visualizes the sensor readings and is used to
demonstrate different use cases around the Smartstrap sensor platform.

**Please note that the backpack is only powered while SensiSmart is open**
Exiting the app will cut the power to the Sensirion Backpack (Smartstrap)


## SensiSmart Software Architecture

The app itself consists of multiple screens, internally called "SensiSmart
apps". The main entry point to the app is located in *SensiSmart.c*. This file
also defines what SensiSmart apps (i.e. screens) are activated by including
their source header and registering them in the AppList enum and apps array.

It is recommended to follow the style of existing SensiSmart apps. They provide
a good reference to extend the app.

### SensiSmart App Framework

The SensiSmart App Framework provides callbacks for events when the app is
loaded initially (*load*) or when the user switches to or away from a SensiSmart
app (*activate*/*deactivate*). Documentation is provided directly in
*SensiSmartApp.h*

If a SensiSmart App implements more than one screen custom button handling must
be added to make sure that the user can get back to the other screens.
When the SensiSmart App consist of a single screen.

### Communication with the Sensirion Backpack

All communication with the Sensirion Backpack happens through the backpack
library. The header *backpack.h* provides the necessary documentation.
*SensiSmart.c* already takes care of initializing the backpack.

To ensure that all SensiSmart apps work well together, please make sure to call
*bp_unsubscribe()* when the *deactivate* callback is issued, such that no
polling of the backpack happens after leaving a SensiSmart app (screen).

### Pebble specifics

Pebble's printf implementation does not provide support for the %f formatter.
*utils.h* provides a *ftoa* implementation to convert floats to a string, that
can then be used with %s.

