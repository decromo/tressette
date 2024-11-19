#!/usr/bin/env bash

set -euo pipefail
numeri=(Asso Due Tre Quattro Cinque Sei Sette Otto Nove Dieci)
semi=(denari coppe spade bastoni)

set +e
rm -rf pages
set -e

let "i = 1"
for s in ${semi[@]}; do
    for num in $(seq 1 10); do
        # wget -e robots=off -P pages "https://commons.wikimedia.org/wiki/File:$(echo -n "00$i" | tail -c 2)_${numeri[((10#$num-1))]}_di_${s}.jpg" || wget -e robots=off -P pages "https://commons.wikimedia.org/wiki/File:$(echo -n "00$i" | tail -c 2)_${numeri[((10#$num-1))]}_di_${s^}.jpg"
        # let "i++";
        # sleep 1
        name1="File:$(echo -n "00$i" | tail -c 2)_${numeri[(($num-1))]}_di_${s}.jpg"
        name2="File:$(echo -n "00$i" | tail -c 2)_${numeri[(($num-1))]}_di_${s^}.jpg"
        name3="File:$(echo -n "00$i" | tail -c 2)_${numeri[(($num-1))]}_di_${s,}.jpg"
        fname="$(
            { wget -e robots=off -P pages "https://commons.wikimedia.org/wiki/$name1" && echo $name1; } ||
            { wget -e robots=off -P pages "https://commons.wikimedia.org/wiki/$name2" && echo $name2; } ||
            { wget -e robots=off -P pages "https://commons.wikimedia.org/wiki/$name3" && echo $name3; })"
        # echo "$(grep fullImageLink pages/$fname)"
        wget -nc -w 1 -e robots=off --output-document downloaded_wiki_images/"$(echo -n "00$num" | tail -c 2)_${s,,}.jpg" -P downloaded_wiki_images $(grep fullImageLink "pages/$fname" | sed 's/^.*><a href="//'| sed 's/".*$//')
        let "i++";
        sleep 1
    done
done

set +e
rm -rf pages
set -e

# wget -r -l 1 -e robots=off -w 1 http://commons.wikimedia.org/wiki/Crystal_Clear
# https://commons.wikimedia.org/wiki/File:01_Asso_di_denari.jpg

# wget -r -l 1 -e robots=off -w 1 http://commons.wikimedia.org/wiki/Crystal_Clear

# WIKI_LINKS=`grep fullImageLink pages/File\:* | sed 's/^.*><a href="//'| sed 's/".*$//'`
# wget -nc -w 1 -e robots=off -P downloaded_wiki_images $WIKI_LINKS