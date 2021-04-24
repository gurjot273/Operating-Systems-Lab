awk '{ if($1 > $2) print $2 " " $1; else if($1 < $2) print $1 " " $2}' < "1f.graph.edgelist" | sort | uniq -u > "1f_output.graph.edgelist.txt"
echo "1f_output.graph.edgelist.txt created successfully"
awk '{print $1; print $2;}' < "1f_output.graph.edgelist.txt" | sort -n | uniq -c | sort -k1 -n -r > "1f_temp.txt"
i=0
echo "The nodes with highest undirected degree are"
while [ "$i" -lt "5" ] ; do
    read line 
    echo $line | awk '{print $2 " " $1}' 
    i=$(( $i +1 ))
done < "1f_temp.txt"
rm "1f_temp.txt"