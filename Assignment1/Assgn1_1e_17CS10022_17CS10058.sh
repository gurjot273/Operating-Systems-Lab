echo "Enter the input file name with extension such as input.txt"
read filename
echo "Enter the column number from 1 to 4"
read col
awk -v col="$col" '{print tolower($col)}' "${@--}" < $filename > "1e_output_${col}_column.freq" 
cat "1e_output_${col}_column.freq" | tr -d ' '| sort | uniq -c > "temp.txt"
mv "temp.txt" "1e_output_${col}_column.freq"
awk '{print $2 " " $1}' < "1e_output_${col}_column.freq"  > "temp.txt"
mv "temp.txt" "1e_output_${col}_column.freq"
sort -k2 -n -r "1e_output_${col}_column.freq" -o "temp.txt"
mv "temp.txt" "1e_output_${col}_column.freq"
echo "1e_output_${col}_column.freq created successfully"