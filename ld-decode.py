#!/usr/bin/env python3
from base64 import b64encode
import copy
from datetime import datetime
import getopt
import io
from io import BytesIO
import os
import sys
import argparse
import json
import traceback

from lddutils import *
import lddecode_core
from lddecode_core import *

options_epilog = """FREQ can be a bare number in MHz, or a number with one of the case-insensitive suffixes Hz, kHz, MHz, GHz, fSC (meaning NTSC) or fSCPAL."""
parser = argparse.ArgumentParser(description='Extracts audio and video from raw RF laserdisc captures', epilog=options_epilog)
parser.add_argument('infile', metavar='infile', type=str, help='source file')
parser.add_argument('outfile', metavar='outfile', type=str, help='base name for destination files')
parser.add_argument('-s', '--start', metavar='start', type=int, default=0, help='rough jump to frame n of capture (default is 0)')
parser.add_argument('-S', '--seek', metavar='seek', type=int, default=-1, help='seek to frame n of capture')
#parser.add_argument('-E', '--end', metavar='end', type=int, default=-1, help='cutting: last frame')
parser.add_argument('-l', '--length', metavar='length', type=int, default = 110000, help='limit length to n frames')
parser.add_argument('-p', '--pal', dest='pal', action='store_true', help='source is in PAL format')
parser.add_argument('-n', '--ntsc', dest='ntsc', action='store_true', help='source is in NTSC format')
#parser.add_argument('-c', '--cut', dest='cut', action='store_true', help='cut (to r16) instead of decode')
parser.add_argument('-m', '--MTF', metavar='mtf', type=float, default=None, help='mtf compensation multiplier')
parser.add_argument('--MTF_offset', metavar='mtf_offset', type=float, default=None, help='mtf compensation offset')
parser.add_argument('-j', '--NTSCJ', dest='ntscj', action='store_true', help='source is in NTSC-J (IRE 0 black) format')
parser.add_argument('--noDOD', dest='nodod', action='store_true', default=False, help='disable dropout detector')
parser.add_argument('--noEFM', dest='noefm', action='store_true', default=False, help='Disable EFM front end')
parser.add_argument('--daa', dest='daa', action='store_true', default=False, help='Disable analog audio decoding')
parser.add_argument('--ignoreleadout', dest='ignoreleadout', action='store_true', default=False, help='continue decoding after lead-out seen')
parser.add_argument('--verboseVITS', dest='verboseVITS', action='store_true', default=False, help='Enable additional JSON fields')

parser.add_argument('-t', '--threads', metavar='threads', type=int, default=5, help='number of CPU threads to use')

parser.add_argument('-f', '--frequency', dest='inputfreq', metavar='FREQ', type=parse_frequency, default=None, help='RF sampling frequency in source file (default is 40MHz)')
parser.add_argument('--video_bpf_high', dest='vbpf_high', metavar='FREQ', type=parse_frequency, default=None, help='Video BPF high end frequency')
parser.add_argument('--video_lpf', dest='vlpf', metavar='FREQ', type=parse_frequency, default=None, help='Video low-pass filter frequency')


args = parser.parse_args()
#print(args)
filename = args.infile
outname = args.outfile
firstframe = args.start
req_frames = args.length
vid_standard = 'PAL' if args.pal else 'NTSC'

if args.pal and args.ntsc:
    print("ERROR: Can only be PAL or NTSC")
    exit(1)

try:
    loader = make_loader(filename, args.inputfreq)
except ValueError as e:
    print(e)
    exit(1)

system = 'PAL' if args.pal else 'NTSC'
    
ldd = LDdecode(filename, outname, loader, analog_audio = not args.daa, digital_audio = not args.noefm, system=system, doDOD = not args.nodod, threads=args.threads)
ldd.roughseek(firstframe * 2)

if system == 'NTSC' and not args.ntscj:
    ldd.blackIRE = 7.5
    
#print(ldd.blackIRE)

if args.seek != -1:
    ldd.seek(firstframe, args.seek)

if args.MTF is not None:
    ldd.rf.mtf_mult = args.MTF

if args.MTF_offset is not None:
    ldd.rf.mtf_offset = args.MTF_offset

DecoderParamsOverride = {}
if args.vbpf_high is not None:
    DecoderParamsOverride['video_bpf_high'] = args.vbpf_high * 1000000

if args.vlpf is not None:
    DecoderParamsOverride['video_lpf_freq'] = args.vlpf * 1000000

if len(DecoderParamsOverride.keys()):
    ldd.demodcache.setparams(DecoderParamsOverride)

if args.verboseVITS:
    ldd.verboseVITS = True

def write_json(ldd, outname):
    jsondict = ldd.build_json(ldd.curfield)
    
    fp = open(outname + '.tbc.json.tmp', 'w')
    json.dump(jsondict, fp, indent=4 if args.verboseVITS else None)
    fp.write('\n')
    fp.close()
    
    os.rename(outname + '.tbc.json.tmp', outname + '.tbc.json')

done = False

while not done and ldd.fields_written < (req_frames * 2):
    try:
        f = ldd.readfield()
    except KeyboardInterrupt as kbd:
        print("Terminated, saving JSON and exiting", file=sys.stderr)
        write_json(ldd, outname)
        ldd.close()
        exit(1)
    except Exception as err:
        print("ERROR - please paste the following into a bug report:", file=sys.stderr)
        print("current sample:", ldd.fdoffset, file=sys.stderr)
        print("arguments:", args, file=sys.stderr)
        print("Exception:", err, " Traceback:", file=sys.stderr)
        traceback.print_tb(err.__traceback__)
        write_json(ldd, outname)
        ldd.close()
        exit(1)

    if f is None or (args.ignoreleadout == False and ldd.leadOut == True):
        done = True

#    print(ldd.fields_written)

    if ldd.fields_written < 100 or ((ldd.fields_written % 500) == 0):
        #print('write json')
        write_json(ldd, outname)

print("saving JSON and exiting", file=sys.stderr)
write_json(ldd, outname)
ldd.close()
