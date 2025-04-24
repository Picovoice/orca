#!/bin/sh

mkdir -p "${SRCROOT}/models/"

rm "${SRCROOT}/models/"*

if [ $1 != 'en_female' ];
then
    cp "${SRCROOT}/../../../lib/common/orca_params_$1.pv" "${SRCROOT}/models/"
fi
