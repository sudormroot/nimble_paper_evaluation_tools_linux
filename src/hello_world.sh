#!/bin/sh

#
# Disable THP migration
# Set maximum memory size to 2048MB
# Set fast memory size to 128MB
#

./launch_testee.sh --thp_migration=1 --max-mem-size=2048 --fast-mem-size=128 --migration-threads-num=1 find /usr

