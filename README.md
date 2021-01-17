# LKM Thread Syncher and Messaging Subsystem
## Introduction
The following module allows threads from different group to synchronize and exchage messages between each others. The project was developed for the *"Advanced Operating System and Virtualization"* course held at Sapienza University. The complete specifications can be found [here](project_specification.md).

The complete **documentation** written in Doxygen can be found [here](https://gr3yc4t.github.io/thread_syncher_messenger/html/index.html)

## Installation
To use the module correctly, the following `udev` rule should be installed on the system by creating a file inside `/etc/udev/rules.d/` (e.g. `99-thread-synch.rules`) with the following content:
```
ACTION=="add", DEVPATH=="/module/aosv2020" SUBSYSTEM=="module" RUN+="/bin/mkdir /dev/synch"
ACTION=="remove", DEVPATH=="/module/aosv2020" SUBSYSTEM=="module" RUN+="/bin/rmdir /dev/synch"
KERNEL=="synch/group[0-9]*", SYMLINK+="group%n_synch_link", GROUP="synch", MODE="0770"
KERNEL=="main_thread_synch", GROUP="synch", MODE="0770"
```
These rules essentially moves all the group's device files inside the `/dev/synch` subdirectory and change device's permission to allows every user that belongs to the `synch` group to access the module's facilities.

### Compiler Options
The module can be customized via the following compiler options:
* `LEGACY_FLUSH`: executes the `cancelDelay` functionality when the group’s device file is flushed (may cause bugs)
* `DISABLE_DELAYED_MSG`: remove all the structures related to the delayed message delivery feature
* `DISABLE_SYSFS`: remove the sysfs interface of the module
* `DISABLE_THREAD_BARRIER`: disable functionality that allows threads to sleep or awake other threads in the same group

The module can be compiled via the `make` command (use `make release` to disable debug information and enable compiler optimization).

## Benchmarking and Testing
The user level tool included in the `tool/` subdirectory can be used for benchmarking purpose: in case an `INI` file is passed as argument, the tool will parse it and execute the corresponding actions.
These feature can be also useful for fuzzing purpose, in this case compile the tool with `afl-gcc` and create some `INI` files for initial testcases.