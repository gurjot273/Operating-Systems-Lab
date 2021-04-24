i=0
if [ ! -d "1.d.files.out" ]
then
    mkdir "1.d.files.out"
fi
echo "Sorting individual files"
while [ "$i" -lt "875" ]
do
    sort -r -n "1.d.files/1.d.files/$i.txt" -o "1.d.files.out/$i.txt"
    i=$(( $i + 1))
done
echo "Individual files and written to 1.d.out folder"
echo "Merging files"
files="1.d.files.out/0.txt"  
i=1
while [ "$i" -lt "875" ]
do
    files="$files 1.d.files.out/$i.txt"
    i=$(( $i + 1))
done
sort -r -m -n $files -o "1.d.files.out/1.d.out.txt"
echo "Final sorted file written to 1.d.files.out/1.d.out.txt"