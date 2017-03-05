#!/bin/sh
# this is an example-script for a noad-call with
# different parameters for a call before or after 
# a recording is done
# this script should be called from inside vdr via '-r ' 
# e.g. vdr '-r /usr/local/sbin/noadcall.sh'

# set the noad-binary here
NOAD="/usr/bin/noad"

# set the online-mode here 
# 1 means online for live-recording only
# 2 means online for every recording
ONLINEMODE="--online=1"

# set additional args for every call here here
ADDOPTS="--ac3 --overlap --jumplogo --comments"

# set special args for a call with 'before' here
# e.g. set a specail statistikfile
BEFOREOPTS="--statisticfile=/video0/noadonlinestat"

# set special args for a call with 'after' here
# e.g. backup the marks from the online-call before
#      so you can compare the marks and see
#      how the marks vary between online-mode 
#      and normal scan (backup-marks are in marks0.vdr)
AFTEROPTS="--backupmarks --statisticfile=/video0/noadstat"

# set the dir of the epg images
EPGIMG_DIR="/video0/epgimages"

echo "noadcall.sh $*" >> /tmp/noad.log
echo "$NOAD $* $ONLINEMODE $ADDOPTS $AFTEROPTS" >> /tmp/noad.log

case "$1" in
  before)
    $NOAD $* $ONLINEMODE $ADDOPTS $BEFOREOPTS
  ;;
  after)
    # Try to copy the EPG-Image to Recording subdir

    EVENTID=`cat $2/info.vdr | sed -n -e 's/^E //p'| awk -F " " '{print $1 }'`
    echo "--------------------" >> /tmp/noadcall.log

    if [ -f $EPGIMG_DIR/$EVENTID.png ]; then
      cp $EPGIMG_DIR/$EVENTID.png $2/thumbnail_0.png
    fi

    n=1

    for i in $EPGIMG_DIR/${EVENTID}_*.png; do
      if [ -f $i ]; then
        THUMB=thumbnail_$n.png
        cp $i $2/$THUMB
        n=`expr $n + 1`
      fi
    done

    # .. other Noad/sharemarks stuff
   
    $NOAD $* $ONLINEMODE $ADDOPTS $AFTEROPTS
  ;;
  edited)
   # FSK protection beim verschieben uebernehmen

   fsk=`echo $2 | sed s/"%"/""/ `
   fsk=$fsk/protection.fsk
   fsknew=$2/protection.fsk

   if [ -f $fsk ]; then
     touch $fsknew
   fi

   # Hier muss das Bildchen verschoben werden, VDR macht nicht

   OLDREC=`echo $2 | sed s/"%"/""/ `

   if [ -f $OLDREC/thumbnail.png ]; then
     cp $OLDREC/thumbnail.png $2/
   fi
  ;;
  rename)
    # Nothing to do.
  ;;  
  move)
    # Nothing to do.
  ;;
  delete)
    # Nothing to do.
  ;;
  *)
    # Nothing to do.
  ;;
esac

