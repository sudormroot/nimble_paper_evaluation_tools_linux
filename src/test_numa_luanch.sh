#!/bin/sh

sudo mkdir /sys/fs/cgroup/test 2>/dev/zero

sudo ./numa_launch --cgroup=/sys/fs/cgroup/test --cpu-node=1 --fast-mem-size=256  --fast-mem-node=1 --slow-mem-node=0 python ~/resnet-in-tensorflow/cifar10_train.py
