rm output-reader-pref.txt
gcc rwlock-reader-pref.c -o reader-pref -lpthread
./reader-pref $1 $2