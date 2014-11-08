#!/bin/bash
cd ..
cvs update -d
cd build/
ant clean && ant
