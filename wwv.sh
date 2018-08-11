#!/bin/sh
wwvsim -u 3 | modulate -v -m am -f -48000 | iqplay -v -f 10048000 -R iq.wwv.mcast.local

