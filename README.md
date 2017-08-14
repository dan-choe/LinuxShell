# Linux Shell

### What is Linux Shell?

This program is made same as **a shell** in low-level Unix/POSIX system.
I implemented this by using existing C libraries and system calls related
to processes, file access, and interprocess communication (pipes and redirection).

Even the user types simple builtin commands, it is forked and called in a child process in order to redirect their output.

### Screenshots

![screenshot1](https://github.com/dan-choe/LinuxShell/blob/master/screenshot1.png "LinuxShell 1")

![screenshot2](https://github.com/dan-choe/LinuxShell/blob/master/screenshot2.png "LinuxShell 2")


### Some sample command lines. This shell covers more programs.
**Execution**
* ls
* cd
* cd ..
* cd -
* pwd

**Piping/Redirection**
* cat sample.txt | grep foo
* fortune | cowsay | cowsay
* echo "Hello World" > output.txt
* echo -e "hello \nworld \ngoodbye \nworld" | grep world
* help | grep cd
* cat < foo.txt > fooout.txt
* cat < input.txt

**Job Control**
* Ctrl + Z
* kill 24 [pid of the last infiniteloop] ? jobs



## License
Copyright [2017] [Dan Choe](https://github.com/dan-choe)
