# Linux Shell

### What is Linux Shell?
This program is made same as **a shell** in low-level Unix/POSIX system.
I implemented this by using existing C libraries and system calls.

### Screenshots

![screenshot1](https://github.com/dan-choe/LinuxShell/blob/master/screenshot1.png "LinuxShell 1")

![screenshot2](https://github.com/dan-choe/LinuxShell/blob/master/screenshot2.png "LinuxShell 2")


### Sample of some command lines. This shell covers more programs.
**Execution**
* ls
* cd
* cd ..
* cd -
* pwd

**Piping/Redirection**
* cat sample.txt | grep foo
* fortune | cowsay | cowsay
* echo HelloWorld
* help | grep cd
* cat < foo.txt > fooout.txt

**Job Control**
* Ctrl + Z
* kill 24 [pid of the last infiniteloop] ? jobs

## License
Copyright [2017] [Dan Choe](https://github.com/dan-choe)
