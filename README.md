# Sensirion Backpack Apps

This repository contains the Sensirion [Pebble](https://www.pebble.com/) smart
watch apps for the Sensirion Sensor Smartstrap (Backpack).

For project details see README.md in specific folder:
- SensiSmart/README.md

## Developing for Pebble

Pebble API documentation and some tutorials are available on:
https://developer.pebble.com/docs/

## Installing the Pebble SDK

The Pebble SDK is available from
https://developer.getpebble.com/sdk/

### ArchLinux

Follow the guide in https://developer.getpebble.com/sdk/install/linux/

You will need the following packages:

 - python2-pip
 - python2-virtualenv

**Note:** Since ArchLinux has python3 by default you must use pip2

To update the SDK use:

```Shell
$ pebble sdk install latest
```

## Creating a Project

Creating a new project is done with the pebble command from the SDK:

```Shell
$ pebble new-project hello_world
```
This will create a new folder `hello_world`.

To compile this project use:

```Shell
$ cd hello_world
$ pebble build
```

## Running on the Target

The easiest way is to download with the cloudpebble. For this you need to use
the pebble app on a phone and pair the phone with your cloudpebble account.
Alternatively, if the phone's IP is reachable from the PC the login step can be
skipped and all instances of --cloudpebble can be replaced with --phone [IP] in
the commands below. When using adb port forwarding (as shown in the next
section) the IP becomes localhost.

### Direct Android phone connection via adb

Run this command to enable adb port forwarding:

```Shell
$ adb forward tcp:9000 tcp:9000
```

You may then use --phone localhost instead of the phone's ip address.

### Direct bluetooth connection

It's possible to connect to the Pebble with the bluetooth from your laptop.
These steps assume a Linux setup:

 * Use the blueman-manager or similar to pair with the Pebble. Make sure you
   pair with the "Pebble Time xxxx" and *not* with "Pebble Time LE xxxx"
   ![blueman-manager screenshot](doc/blueman_manager.png)
 * Start the Serial Port Server. This should create a device like /dev/rfcomm0
   ![blueman-manager screenshot](doc/blueman_manager_2.png)
 * Use the `--serial /dev/rfcomm0` command line switch to use the connection


### Cloudpebble Login

To pair the SDK with cloudpebble (needed for cloud connection to phone)
Use `pebble login` and open the displayed URL with your browser and generate an
API token:

```Shell
$ pebble login --noauth_local_webserver
Go to the following link in your browser:

    https://auth.getpebble.com/oauth/xxxx

    Enter verification code: xxxxxxxxx
	Authentication successful.
```

### Installation and Logs

Then use (adb forwarding first line, cloudpebble second line):
```Shell
$ pebble install --phone localhost
$ pebble install --cloudpebble
```
To download the app to your watch.

To view the logs use one of the following "pebble logs" commands
```Shell
$ pebble logs --phone localhost # Pebble logs via adb forwarding
$ pebble logs --cloudpebble     # Pebble logs via cloudpebble
```

## App Deployment

Built apps (.pbw files) can be distributed by sending (e.g. by email) them
directly to the smartphone for installation (Android and iOS devices with the
Pebble app). Opening the file will then prompt the Pebble App to launch and
install the app on the paired Pebble.
The Pebble app archive (.pbw file) is created in the build folder of the project.
