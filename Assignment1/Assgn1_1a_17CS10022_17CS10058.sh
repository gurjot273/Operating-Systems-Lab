if [ "$2" == "+" ]
then
    echo $(($1+$3))
elif [ "$2" == "-" ]
then
    echo $(($1-$3))
elif [ "$2" == "*" ]
then
    echo $(($1-$3))
elif [ "$2" == "/" ]
then
    if [ "$3" -eq "0" ]
    then
        echo "Division by zero not allowed"
    else
        echo $(($1/$3))
    fi
elif [ "$2" == "%" ]
then
    if [ "$3" -eq "0" ] 
    then
        echo "Modulus by zero not allowed"
    else
        echo $(($1%$3))
    fi
fi