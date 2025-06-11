# test

This directory contains test resources.

# Test scenarios

There are several start scripts that bring up entire test systems to
test integration of particular components.

All scenarios facilitate `tmux` to orchestrate several processes.

BEFORE bringing up *any* test scenario, a running `tmux` server is required,
thus start one:

```
>    tmux
```

Once this is done, just cd into a scenario directory, and execute the python start
script:

```
python3 start.py
```

This will get you into a tmux session running all the processes.
They should have been set up in a way that you allways should hear sample output
.

You can kill the running test session by going to a different console, cd to
the test scenario directory and start the start script using the `--kill` switch:

```
python3 start.py --kill True
```

The test scenario directories are numbered, in ascending order from simple to
more complex scenarios.

The more complex scenarios rely on the proper execution of the simpler scenarios.

This means that when testing the entire backend, it is advisable to execute first
`01-rtp_cli` and only afterwars `02-...` followed by `03-...` etc.

## 01-rtp_cli

Test scenario consisting of 2 rtp clis, where one send to the other.
the receiver outputs the received signal to the audio card.

Bringing them up is straight-forward:

```
cd 01-rtp_cli
python3 start.py
```

By default, the sender generates a sinus signal, but you can also replay 
a pcm16s raw file by:

```
cd 01-rtp_cli
python3 start.py --infile abs_path_to_pcm_file
```

You can stop the running session by going to a second terminal and do a

```
cd 01-rtp_cli
python3 start.py --kill True
```

## 02-mixer

Starts a scenario for testing a mixer.
Consists of

* 1 rtp_cli that receives rtp and outputs payload to sound card
* 1 mixer ss
* 1 mixer that sends to the receiver
* 2 rtp sender that send to the mixer

Configuration is done via `02-mixer/configuration.py`

There `INFILE1` and `INFILE2` could be set to a path for a pcm file to replay

#  03-echo_cancellator

Scripts to set up a simple backend consisting of

* one EC
* one Mixer
* 2 rtp_cli s that send test signals
* 1 rtp_cli that receives and outputs signals

# 04-resmgr

Scripts to set up a simple backend consisting of (in the default configuration):

* 3 mixer
* 2 EC
* 2 rtp_cli s that send signals
* 1 rtp_cli that receives/outputs signals
* a resource manager to manage the mixer/ec

to test the resource manager (and hence the entire complexity of the audio
backend)

By default, 2 users ('ymir' and 'buri') as well as one loop ('ratatoskr')
are set up, but the users / loops to set up can be configured by editing
in `configuration.py` either

* the `USERS` dict or
* the `LOOPS` array

The users are not configured to talk nor to listen, but they can be easily
switched into either by just going to the tmux window `ss_liege` and typing in
appropriate commands:

* for listen: `msg listen_on user=xxx loop=yyy`
* for talk: msg talk_on user=xxx loop=yyy`

I won't go into details on how the SS works, it is in a way self-documenting:

Type

* `help` to get a basic overview of the provided commands,
* `requests` to view the requests supported by the `msg` command and
* `request REQUEST_NAME` for help on a specific request

# Mixing pipeline

This provides a python3 script that takes several integer raw 16bit be audio files, and sets up senders/a mixer and a receiver to send them to the mixer and have them mixed together.
Mixed result is written as raw 16bit BE audio.

# audio_samples

Like the name says.
The audio samples are produced using `espeak(1)` and `mplayer(1)`
Also contains a utility to strip RIFF/WAVE container from audio payload

Type

```
make
```

to build actual audio files.

You can just drop WAVE or Text files into the folder, they will be converted to
RAW PCM suited for being read by ov_rtp_cli or otherwise fed directly into the
backend
