#!/usr/bin/env python3

import os
import sys
import subprocess
import json

class File():

    def __init__(self, path):
        self.path = path
        self.read_ops = 0
        self.read = 0
        self.write_ops = 0
        self.write = 0

    def doread(self, size):
        self.read_ops = self.read_ops + 1
        self.read = self.read + size

    def dowrite(self, size):
        self.write_ops = self.write_ops + 1
        self.write = self.write + size

    def stats(self):
        print("[{}]".format(self.path))
        print("    READ  : {} operations for {} bytes".format(self.read_ops, self.read))
        print("    WRITE : {} operations for {} bytes".format(self.write_ops, self.write))



class EventParser():
    def __init__(self, fd):
        self.fd = fd
        self.files = {}
        self.fds = {}
        self._reg_std_fds()

    def _reg_std_fds(self):
        self.files["stdout"] = File("stdout")
        self.files["stderr"] = File("stderr")
        self.files["stdin"] = File("stdin")
        self.fds["0"] = self.files["stdin"]
        self.fds["1"] = self.files["stdout"]
        self.fds["2"] = self.files["stderr"]

    def _read_one_event(self):
        data = bytearray()
        while 1:
            c = os.read(self.fd,1)
            #print(c)
            if c == b'':
                return None
            if c == b'\x00':
                break
            data += c

        ddata = data.decode('utf8')

        return json.loads(ddata)


    def _proces_event(self, event):
        scall = event["syscall"]["name"]
        if scall == "open" or scall == "openat":
            path = event["syscall"]["args"]["path"]
            fd = event["syscall"]["ret"]
            if fd == "-1":
                return
            if path not in self.files:
                self.files[path] = File(path)
            if fd not in self.fds:
                self.fds[fd] = self.files[path]
        if scall == "close":
            fd = event["syscall"]["args"]["int"]
            if fd in self.fds:
                del(self.fds[fd])
        if scall == "read":
            fd = event["syscall"]["args"]["fd"]
            if fd in self.fds:
                f = self.fds[fd]
                f.doread(event["syscall"]["ret"])
        if scall == "write":
            fd = event["syscall"]["args"]["fd"]
            if fd in self.fds:
                f = self.fds[fd]
                f.dowrite(event["syscall"]["ret"])

    def read_events(self):
        while 1:
            ev = self._read_one_event()
            if ev is None:
                break
            self._proces_event(ev)



    def stats(self):
        for f in self.files.values():
            f.stats()




def run():

    if len(sys.argv) == 1:
        raise Exception("A command is expected")

    profile_data_pipe = os.pipe()

    mydir = os.path.dirname(os.path.realpath(__file__))

    driver_cmd = ["{}/perf".format(mydir), "-i", "-j", "-r", str(profile_data_pipe[1]), "--"]

    command = sys.argv[1:]

    ep = EventParser(profile_data_pipe[0])

    sp = subprocess.Popen(driver_cmd + command, pass_fds=[profile_data_pipe[1]])

    os.close(profile_data_pipe[1])
    ep.read_events()

    sp.wait()

    ep.stats()

run()