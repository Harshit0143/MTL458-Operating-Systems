rm output-writer-pref.txt
gcc rwlock-writer-pref.c -o writer-pref -lpthread
./writer-pref $1 $2