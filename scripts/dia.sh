
INTERVAL=10
IMAGE_DIR="/ablage/Pictures/"
REPEAD=yes
# MOUNTPOINT=/pub/images/Pictures

FILE="/tmp/dia.file"
LIST="/tmp/dia.list"

# ---------------------------------------------------
# stop ??
# ---------------------------------------------------

if [ "$1" == "stop" ]; then
   svdrpsend.pl plug graphtft VIEW Standard
   rm "$LIST"
   exit 0
fi

# ---------------------------------------------------
#  check mount point (if required)
# ---------------------------------------------------

if [ -n "$MOUNTPOINT" ]; then
   check-mount.sh "$MOUNTPOINT"

   if [ "$?" == 1 ]; then
      echo mount failed aborting diashow
      exit 1
   fi
fi

# ---------------------------------------------------
#  create list 
# ---------------------------------------------------

echo creating image list
find $IMAGE_DIR -name *.jpg -o -name *.JPG > "$LIST" 

# ---------------------------------------------------
#  switching mode
# ---------------------------------------------------

svdrpsend.pl plug graphtft VIEW Dia

# ---------------------------------------------------
#  loop
# ---------------------------------------------------

echo starting show ...

while [ -f "$LIST" ]; do

   cat "$LIST" | while read i; do 

     # skip thumbnails
 
      echo $i | grep "/\.pics/"

      if [ "$?" == 1 ]; then
        echo $i 
        echo $i > "$FILE"
        svdrpsend.pl PLUG graphtft REFRESH 2>&1 > /dev/null

        sleep $INTERVAL 
      else
        echo skipping $i
      fi

      if [ ! -f "$LIST" ]; then
         break
      fi

   done

   if [ "$REPEAD" != "yes" ]; then
      break
   fi

done

rm -f "$LIST" "$FILE"
exit 0

