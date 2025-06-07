#!/usr/bin/python3

import os
from pipe import *

class Mux:
    def __init__(self, pipe_dir):
        self.pipe_dir = pipe_dir
        self.pipes = []

    def cycle(self):
        self.scan()
        self.updatePipes()

    def scan(self):
        new_pipes = os.listdir(self.pipe_dir)

        for p in self.pipes:
            if not p.name in new_pipes:
                print(f"Removed {p.name}")
                self.pipes.remove(p)

        for p in new_pipes:
            if not p in self.getMatchingPipeNames(p):
                path = self.pipe_dir + "/" + p
                self.pipes.append(Pipe(p, path))
                print(f"Added {p}")

    def updatePipes(self):
        for p in self.pipes:
            if p.pipe_type == PipeType.INPUT:
                self.processPipe(p)

    def getMatchingPipeNames(self, name):
        ps = []
        for p in self.pipes:
            if p.name == name:
                ps.append(p.name)

        return ps

    def getMatchingPipeTypes(self, pipe):
        ps = []
        for p in self.pipes:
            if p.type == pipe.type and p.pipe_type == PipeType.OUTPUT:
                ps.append(p)

        return ps

    def processPipe(self, p):
        t = self.getMatchingPipeTypes(p)

        d = p.read()

        if d == None:
            return

        print(f"[{p.name}] {d} => ", end="")

        for op in t:
            print(op.name, end=" ")
            op.write(d)
        
        print()

m = Mux("pipes")

while 1:
    m.cycle()
