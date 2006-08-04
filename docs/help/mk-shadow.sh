#!/bin/bash
# based on
# http://joakim.erdfelt.com/thoughts/archives/2-Creating-Drop-Shadows-with-netpbm.html#extended
# 
# pngtopnm flower-pink.png | ./pnmdropshadow.sh | pnmtopng -compression=9 > flower-pink-with-shadow.png
#
# for PNG in *.png
# do 
#   echo Adding drop shadow to $PNG
#   pngtopnm $PNG | ./pnmdropshadow.sh | pnmtopng -compression=9 > ${PNG//.png}-with-shadow.png
# done

TEMPDIR=".pnmdropshadow"

if [ ! -d "$TEMPDIR" ] ; then
    mkdir -p $TEMPDIR
fi

function pnmWidth() {
    PNM=$1
    
    pamfile $PNM | \
        sed -e "s/^.* \([0-9][0-9]*\) by [0-9][0-9]* .*$/\1/g"
}

function pnmHeight() {
    PNM=$1
    
    pamfile $PNM | \
        sed -e "s/^.* [0-9][0-9]* by \([0-9][0-9]*\) .*$/\1/g"
}

function pnmBorder() {
    PNM=$1

    COLOR="#555555"

    # Add Border to PNM
    pnmmargin -color "$COLOR" 1 $PNM > $TEMPDIR/with_border.pnm

    cp $TEMPDIR/with_border.pnm $PNM
}

function createConvolMap() {
    let blurSize=$1;

    if [ `expr $blurSize % 2` -eq 0 ] ; then
        let blurSize=$blurSize+1;
    fi

    let offset=$blurSize*$blurSize;
    let rowvalue=$offset+1;
    let offset=$offset*2;

    echo "P2"
    echo "$blurSize $blurSize"
    echo "$offset"

    for (( y=0; y<$blurSize; y++))
    do
        for (( x=0; x<$blurSize; x++))
        do
            echo -n "$rowvalue"
            if [ $x -ne $blurSize ]
            then
                echo -n " "
            fi
        done
        echo ""
    done
}

function pnmDropShadow() {
    PNM=$1
    
    BGCOLOR="#ffffff"
    SHADOWCOLOR="#444444"
    
    let shadowOffset=3;
    let shadowBlur=8;

    let leftPad=$shadowBlur-$shadowOffset;
    if [ $leftPad -lt 0 ] ; then
        let leftPad=0;
    fi
    let leftPad=$leftPad+$leftPad;
    let rightPad=$shadowBlur+$shadowOffset;
    if [ $rightPad -lt 0 ] ; then
        let rightPad=0;
    fi
    let rightPad=$rightPad+$rightPad;
    let topPad=$shadowBlur-$shadowOffset;
    if [ $topPad -lt 0 ] ; then
        let topPad=0;
    fi
    let topPad=$topPad+$topPad;
    let bottomPad=$shadowBlur+$shadowOffset;
    if [ $bottomPad -lt 0 ] ; then
        let bottomPad=0;
    fi
    let bottomPad=$bottomPad+$bottomPad;

    let pnmWidth=`pnmWidth "$PNM"`
    let pnmHeight=`pnmHeight "$PNM"`
    let shadowWidth=$pnmWidth+$leftPad+$rightPad;
    let shadowHeight=$pnmHeight+$topPad+$bottomPad;
    let maskWidth=$pnmWidth+$shadowBlur;
    let maskHeigth=$pnmHeight+$shadowBlur;

    # Create the shadow background
    ppmmake "$BGCOLOR" $shadowWidth $shadowHeight > \
        $TEMPDIR/shadow_background.pnm
    
    # Create the mask block
    ppmmake "$SHADOWCOLOR" $maskWidth $maskHeigth > \
        $TEMPDIR/shadow_mask.pnm
    
    # Insert mask block into the background block
    pnmpaste -replace \
        $TEMPDIR/shadow_mask.pnm \
        $shadowBlur $shadowBlur \
        $TEMPDIR/shadow_background.pnm > \
            $TEMPDIR/shadow_paste.pnm

    # Create Convolution Matrix File
    createConvolMap "$shadowBlur" > $TEMPDIR/shadow.cnv

    # Blur the shadow
    pnmconvol \
        $TEMPDIR/shadow.cnv \
        $TEMPDIR/shadow_paste.pnm > \
            $TEMPDIR/shadow_blur.pnm

    # Put original image into blur
    pnmpaste -replace $PNM \
        $leftPad $topPad $TEMPDIR/shadow_blur.pnm > \
            $TEMPDIR/with_shadow.pnm

    cp $TEMPDIR/with_shadow.pnm $PNM
}

PNMT="$TEMPDIR/temp.pnm"

if [ -z "$1" ]; then
    pnmtopnm > $PNMT
else
    cp $1 $PNMT
fi

#pnmBorder "$PNMT"
pnmDropShadow "$PNMT"

pnmtopnm "$PNMT"

rm -rf $TEMPDIR
