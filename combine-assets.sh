#!/usr/bin/env bash

set -euo pipefail
semi=(denari coppe spade bastoni)

for s in ${semi[@]}; do
    magick convert *_${s^}.png +append sm_${s^}.png
done

magick convert $(IFS=,; eval echo "sm_{${semi[*]^}}.png") -append full.png
magick full.png -resize 50% half.png