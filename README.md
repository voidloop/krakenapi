krakenapi
=========

A C++ library for interfacing with the Kraken REST API (kraken.com).

Other programs
==============

krt
---

Source file of this program is krt.cpp.

### What is krt?

krt is a program to download Recent Trades from Kraken market data through API.  

### How trades are displayed? 
 
Recent trades are printed out to standard output in CSV format. The order of fields is "Time", "Order", "Price" and "Volume".

### Command line arguments

usage: krt <pair> [interval] [since]

### TODO: Command line arguments

krt can get the following command line arguments:

  -p, --pair <pair>
  Asset pair to get trade data for.

  -s, --since <id>
  Return trade data since given id. By default id is equal to "0"
  to indicate the oldest possible data

  -n, --interval <seconds>
  How many seconds the program have to download new trade data. By default 
  the program doesn't use this parameter and it exits after download trade
  data. If <seconds> is equal to 0 the program will not use this parameter.
