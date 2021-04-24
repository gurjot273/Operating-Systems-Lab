filename="1b_input.txt"
n=1
finalfile="1b_out.txt"
echo "Serial Random string" >> $finalfile
while read line; do
# reading each line
echo "$n $line" >> $finalfile
n=$((n+1))
done < $filename
echo "$n $line" >> $finalfile
echo "New numbered file 1b_out.txt created successfully"