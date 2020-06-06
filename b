#!/bin/bash
(cd `readlink -f build/debug` && ninja)
