#!/bin/bash

# From https://stackoverflow.com/questions/59895
script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
echo ${script_dir}

# Original footprint file
src=$1

# Number of 90 deg rotations (CCW on JLCPCB)
n=$2

echo ${src}

temp="$(mktemp).kicad_mod"

last=${src}

i=0
until [ $i -ge $n ]
do
  i=$((i+1))
  next="${temp}.${i}"
  echo "90 deg ccw: ${next}"
  sed -f ${script_dir}/rotate.sed < ${last} > ${next}
  last=${next}

done

cp ${last} ${src}
echo ${src}
