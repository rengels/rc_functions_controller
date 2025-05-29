@page page_architecture Architecture

@tableofcontents

# Architecture documentation for rc_functions_controller

## Introduction

This page gives an overview of the project architecture, loose following the arc42 template.

## Constraints

Currently the project is limited to the ESP32 HW platform.
Other platforms are not supported.
Reasons: We are using HW specific to the esp32 platform like DAC, ADC, PWM, LED, BLE, ...

## Context & Scope

The rc_functions_controller project is meant for non safety critical, private hobby projects.

This document 

## Features

Supported input sources (for a full list see \ref page_procs)

- RC PWM
- PPM
- SBUS (not fully implemented yet)
- IBUS (not fully implemented yet)
- SRXL (Multiplex version tested)
- voltage input (e.g. for low battery warning)

Supported output sources (for a full list see \ref page_procs)

- Two channel 8 bit Sound output (DAC)
- PWM light output signals
- PWM servo output
- ESC output

Other features:

- configuration via Bluetooth LE
- flexible configuration
- engine RPM simulation
- vehicle mass simulation
- gear simulation
- xenon light effect
- incandescent light bulb effect


The following table shows a list of the pins and their functionality.
All pins are optional. If not configured they are not used.

| pin | PWM in | ADC in | PIN in | PPM/SBUS/SRXL in | Audio out | LED out | PWM out | ESC out |
|-----|--------|--------|--------|------------------|-----------|---------|---------|---------|
|   1 |        |        |        |                X |           |         |       X |       X |
|   2 |        |        |        |                X |           | ch. 1   |       X |       X |
|   3 |        |        |        |                X |           | ch. 2   |       X |       X |
|   4 |        |        |        |                X |           | ch. 3   |       X |       X |
|   5 |        |        |        |                X |           | ch. 4   |       X |       X |
|   6 |        |        |        |                X |           |         |       X |       X |
|   7 |        |        |        |                X |           |         |       X |       X |
|   8 |        |        |        |                X |           |         |       X |       X |
|   9 |        |        |        |                X |           |         |       X |       X |
|  10 |        |        |        |                X |           |         |       X |       X |
|  11 |        |        |        |                X |           |         |       X |       X |
|  12 | ch. 1  |        | ch. 1  |                X |           |         |       X |       X |
|  13 | ch. 2  |        | ch. 2  |                X |           |         |       X |       X |
|  14 | ch. 3  |        | ch. 3  |                X |           |         |       X |       X |
|  15 |        |        |        |                X |           | ch. 5   |       X |       X |
|  16 |        |        |        |                X |           | ch. 6   |       X |       X |
|  17 |        |        |        |                X |           | ch. 7   |       X |       X |
|  18 |        |        |        |                X |           | ch. 8   |       X |       X |
|  19 |        |        |        |                X |           | ch. 9   |       X |       X |
|  20 |        |        |        |                X |           |         |       X |       X |
|  21 |        |        |        |                X |           | ch. 10  |       X |       X |
|  22 |        |        |        |                X |           | ch. 11  |       X |       X |
|  23 |        |        |        |                X |           | ch. 12  |       X |       X |
|  24 |        |        |        |                X |           |         |       X |       X |
|  25 |        |        |        |                X | channel 1 |         |       X |       X |
|  26 |        |        |        |                X | channel 2 |         |       X |       X |
|  27 | ch. 4  |        | ch. 4  |                X |           |         |       X |       X |
|  32 |        | X      |        |                X |           | ch. 13  |       X |       X |
|  33 |        | X      |        |                X |           |         |       X |       X |
|  34 | ch. 5  | X      | ch. 5  |                X |           |         |         |         |
|  35 | ch. 6  | X      | ch. 6  |                X |           |         |         |         |
|  36 |        | X      |        |                X |           |         |       X |       X |
|  37 |        | X      |        |                X |           |         |       X |       X |
|  38 |        | X      |        |                X |           |         |       X |       X |
|  39 |        | X      |        |                X |           |         |       X |       X |




## Solution strategy

On a high level the different rc functions are controlled by the rc input.
So we have a clear information flow from RC input via different processing
steps to the output.

Since the controller is intended to be flexible the different steps
(input, processing, output) are handled by interchangable components
connected via a standardized data bus/signaling mechanism.

Most RC inputs are cyclic in the area of 20ms. So it makes sense to
implement a strict static scheduling of 20ms.

### Bluetooth LE services

The controller advertises a Bluetooth LE device with the name "RcFuncCtrl-\<random two character postfix\>" and the manufacturer "OSS".
It provides the following services (in addition to default BLE services):

| UUID | Description | flags |
|------|-----------------|-----|
| 31522e91-04d6-dfae-2042-3a40b42d393f | signals service |  |
| 177d4281-2c71-50b2-264f-3fb3f9b556a8 | signals characteristic | read, write, notify |
| 32522e91-04d6-dfae-2042-3a40b42d393f | configuration service| |
| 187d4281-2c71-50b2-264f-3fb3f9b556a8 | configuration characteristic | read, write |
| 197d4281-2c71-50b2-264f-3fb3f9b556a8 | audio file characterisitc | write |
| 1a7d4281-2c71-50b2-264f-3fb3f9b556a8 | audio list characterisitc | read |

### Signals Characteristics

The signals characteristics will just send/receive and notify a list
of signals.

Writing that characteristics will write an *override* list that will
set signals right at the beginning.

### Configuration Characteristics

We want a flexible configuration mechanism.
To implement this we are using a binary format to transmit the configuration
of the *Proc* components.
For that we use a simple binary stream format that looks like this:

| Byte No | Value | Description |
|---------|-------|-------------|
| 0 | 'R' | Magic number/header |
| 1 | 'C' | Magic number/header |
| 2 | 0x01 | Binary format version |
| 3 | 0x01  | Proc count |
| 4 | 'P' | Proc type ID |
| 5 | 'A' | Proc type ID |
| 6 | 3 | Proc data size |
| 7 | 0x10 | Input type ID 1 |
| 8 | 0x15 | Output type ID 1 |
| 9 | 0x21 | Output type ID 2 |

::Note
    We don't want to use protobuf because of the comperatively high
    effort to set this up (e.g. require protoc and nanopb).
    We also decided against Json because of the message size and
    the size of the necessary Json library.

### Audio File Characteristics

The storage on the esp32 system is pretty large (2MB), but the RC_Engine_Sound project
had even more audio data than that (16MB) in it's samples folder.
Compressing these files would not get us into the right range.

Therefore I decided to implement a list of basic "static" samples and a mechanism
to upload new samples.

This is done via the Audio Characteristics.
It supports the following commands (for now):

| ID | Description | Data
|---------|-------|-------------|
| 0 | Reset all samples | |
| 1 | New Audio | Audio ID, file size|
| 2 | Add to Audio | Audio ID, offset (4 bytes), size (4 bytes), data |

An example audio command message looks like this:

| Byte No | Value | Description |
|---------|-------|-------------|
| 0 | 'R' | Magic number/header |
| 1 | 'A' | Magic number/header |
| 2 | 0x01 | Binary format version |
| 3 | 0x02  | Command type (e.g. add to audio) |
| 4-6 | 'AAA' | Audio ID |
| 7.. | . | Optional parameters depending on the command |

::Note
    Initially I was thinking about a neat scheme to remove, merge and overwrite
    audio samples.
    However, with just a simple mechanism, the code is much less complex
    and we get a "kind of" wear leveling.


### Audio List Characteristics

The audio list characteristics allows querying the current available
custom samples.

This is an example of a query result for the audio list characteristics:

| Byte No | Value | Description |
|---------|-------|-------------|
| 0 | 'R' | Magic number |
| 1 | 'L' | Magic number |
| 2 | 0x01 | Binary format version |
| 3-4 | 0043 | 16 bit Number of used segments (4096 bytes) |
| 5-6 | 0023 | 16 bit Number of free segments |
| 7 | 2 | Number of custom samples |
| 8-10 | 'AAA' | Audio ID |
| 11-14 | 100034 | Big endian uint32 sample size |
| 15-16 | 0x32AF | 16 bit CRC checksum |


## Building block view

The following diagram shows a broad overview of the SW components:

@startuml

package "controller" {
[main]
[ProcStorage]
[SampleStorage]
[SimpleInputBuffer]
[SimpleOutputBuffer]
}

package "rcProc" {
[ProcMath]
[ProcIndicator]
[other procs]
}

package "rcInput" {
[InputDemo]
[InputPwm]
[InputPpm]
[InputAdc]
}

package "rcOutput" {
[OutputPwm]
[OutputLed]
[OutputAudio]
}

package "rcEngine" {
[EngineSimple]
[EngineGear]
[EngineReverse]
}

package "rcAudio" {
[AudioSimple]
[AudioLoop]
}

package "rcSamples" {
[samples]
}

package "bluetooth" {
[Nimble]
}

package "rcSignals" {
[Signals]
}



[main] --> [ProcStorage] : contains
[main] --> [SampleStorage] : contains
[main] ..> Nimble : use

[ProcStorage] ..> [SimpleInputBuffer] : use
[ProcStorage] ..> [SimpleOutputBuffer] : use
[ProcStorage] ..> rcInput : use
[ProcStorage] ..> rcProc : use
[ProcStorage] ..> rcOutput : use
[ProcStorage] ..> rcEngine : use

[EngineGear] --> [EngineSimple] : inherits

[SampleStorage] ..> [SimpleInputBuffer] : use
[SampleStorage] ..> [SimpleOutputBuffer] : use
[SampleStorage] ..> rcAudio : use
[SampleStorage] ..> rcSamples : use

rcProc ..> rcSignals : use
rcInput ..> rcSignals : use
rcOutput ..> rcSignals : use
rcAudio ..> rcSignals : use

@enduml

### controller

The main component of the project.
Contains the *Storage* components which manages all the *Proc* and *Sample* instances.
The *controller* will create a periodic task with the main loop, executing the steps function of all procs.

### rcProc::Proc

See the following file for a list of the available signals:
\ref page_procs

The *rcProc::Proc* nodes do transformations (processing) on the rc signals.
They implement all the necessary functionalities an are split up into

- proc: the _normal_ processing nodes.
- input: HW input and signal generation.
- output: HW output including sound output.
- audio: Audio generation (but not output).

They all have the following functions:

- a *step* function that is called periodically with a list of signals and
  a reference to the audio buffer.
- a *start* and *stop* function that will activate and deactivate the component.
  These functions will also initialize necessary inputs timers and interrupts.

### rcInput::Input

An *rcInput::Input* nodes will read an RC input from the HW ports and set the signals
accordingly.

Although these _procs_ can be placed anywhere in the _proc-stack_, it makes sense
to have them at the front.

### rcOutput::Output

An *rc::Output* component will output *Signals* via different means.

Although these _procs_ can be placed anywhere in the _proc-stack_, it makes sense
to have them at the back.

### rcAudio::Audio

An *rc::Audio* component will add audio data/samples to the audio ring buffer.
These _procs_ are nothing special.
Theoretically every _proc_ could add audio data, modify and create signals.
However in practice only the Audio procs will do that.


## Runtime view

@startuml
start

partition "Initialization" {
    :initialize flash;
    :load procs from nvm;
    :initialize ble nimble stack;
    :initialize bluetooth queues;
}
partition "Main loops" {
    fork
        repeat
            :delay 20ms;
            :initialize signals from bluetooth signals;
            :init audio ringbuffer blocks;
            :call all the step for procs;
            :finalize audio ringbuffer blocks;
            :update bluetooth queues;
        repeat while (forever)
    fork again
        repeat
            :delay 5s;
            :print main task timing info;
        repeat while (forever)
    fork again
        :do ble nimble stuff;
    end fork
}

@enduml


### Signals

See the following file for a list of the available signals:
\ref page_signals


The communication between the different input, output and proc modules
is done via a list of signals.

Most modules do some kind of transformation on the signals and give them
to the output.

The following image gives an example for the "indicator" signals.

@startuml

[rcInput::Input] --> [rcSignals::ProcSwitch] : switch channel is written to ST_AUX1
[rcSignals::ProcSwitch] --> [rcSignals::ProcAuto] : outputs LI signals
[rcSignals::ProcAuto] --> [rcSignals::ProcIndicator] : outputs LO signals
[rcSignals::ProcIndicator] --> [rcSignals::ProcFade]
[rcSignals::ProcFade] --> [rcOutput::OutputLed]

note right of [rcSignals::ProcSwitch]
    The switch proc will decode a switch RC channel.
    In our case it will fill the ST_LI_INDICATOR_LEFT
    ST_LI_INDICATOR_RIGHT and/or ST_LI_HAZARD signal.
    (LI stands for light in)
end note

note right of [rcSignals::ProcAuto]
    ProcAuto combines hazard and indicator input
    signals into ST_LO_INDICATOR_LEFT and
    ST_LO_INDICATOR_RIGHT. (among other things)
end note

note right of [rcSignals::ProcIndicator]
    ProcIndicator will periodically switch the
    signals off for a blinking effect.
end note

note right of [rcSignals::ProcFade]
    ProcFade can apply a fade effect, simulating
    an incandecant light bulb.
end note

note right of [rcOutput::OutputLed]
    The ST_LO_INDICATOR_LEFT signal is send to
    a port via the esp LED module.
end note

@enduml


## Deployment View

Just deploy via Visual Studio Code or Arduino IDE.

## Crosscutting Concepts

## Architectural Decisions

### Flexible configuration

Since the original project was already using WiFi for communication to the
trailer, there is benefits in using the RF components also for the
configuration.

The original project has many issues with setting up the receiver
configuration and then re-compiling it.

### Serialization protocol

Storing and transmitting the configuration requires a way to serialize
it.
Possible solutions are Protobuf, JSON or hand-written code.

Protobuf is a good solution, but it takes some effort to set it up.

Especially since you need nanopb on embedded devices (another dependency).
JSON integrates nicely with Javascript and HTML but produces fairly large messages.

The hand-written solution is really small and fast.
Also it seems like other Bluetooth services use similar binary protocols.

So we are using our own buffers now.


## Quality Requirements

Since this is a hobby project we don't have a lot of quality requirements.
However the source code is using the following coding conventions:

- Try to be MISRA compliant, e.g. curly braces for conditions
- Type safety where possible
- Doc-comments for every function and every non-trivial parameter

Since the project is using dynamic memory it can never be fully MISRA compliant.
Also, since there is no safety architecture the project is not ISO26262 compliant
and thus should never be used in any *real* vehicles.

## Risks & Technical Debt

Currently there are no risks and technical debts.

## Glossary



