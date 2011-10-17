#!/bin/bash

rsync -a --progress --exclude *pcap --exclude scripts tnt@yosemite:exp/* .
