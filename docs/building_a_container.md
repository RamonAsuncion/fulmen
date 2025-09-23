# Building a Container Runtime

A container runtime is the software that loads, runs, and manages containers. No it's not a VM. It's a process that runs on the host machine. 

## Minimal setting up a container

- Pull image tar
- Create a temporary dir
- Extract image into it
- Chroot into it
- Start process
- On exit, delete dir

This only works on Linux.

## How to make this real

We need to add namespaces (similar to programming languages) where if 
you have a global variable everyone can see it but if you have a namespace
then you can create a variable with the same name in two different namespaces.

- Network
- Mount
- PID
- User
- UTS (hostname)
- Cgroupe

## References

- [Build your own Container Runtime](https://www.youtube.com/watch?v=JOsWB50LmwQ) A good talk on creating a runtime from scratch using the Linux chroot syscall