gcd ()
{
  m=$1
  n=$2

  if [ $m -eq 0 ]; then
    return $n
  elif [ $n -eq 0 ]; then
    return $m
  fi

  gcd $(( $n % $m )) $m
}

res=0
echo $1 | { while read -d, line; 
do gcd $res $line;
res=$? ;
done; 
gcd $res $line;
res=$? ;
echo $res;}
