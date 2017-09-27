file=$1

expname=${file%*.*}

tr -d " " < $file > tmp.txt
mv tmp.txt $file
echo $file

echo "Converting raw data to microseconds..."
octave convert_raw_to_usec.oct $expname "."

