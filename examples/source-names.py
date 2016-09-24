#!/usr/bin/python3
# -*- coding: utf-8 -*-
#
# Copyright © Philip Withnall 2015 <philip@tecnocode.co.uk>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by the
# Free Software Foundation; either version 2.1 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
# for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

#
# An example log analysis program for Dunfell. It loads a log (specified as the
# first command line argument) then prints out the names of all the GSources
# mentioned in the log file, as a basic example of performing analysis on a
# log file.
#

import gi
gi.require_version('Dunfell', '0')
gi.require_version('DunfellUi', '0')
gi.require_version('Gio', '2.0')
from gi.repository import Dunfell, DunfellUi, Gio
import sys

parser = Dunfell.Parser.new()
parser.load_from_file(sys.argv[1])
model = parser.dup_model()

sources = model.dup_sources()

# Print out the names of all the sources
for source in sources:
    print(source.props.name if source.props.name else "(Unnamed)")
