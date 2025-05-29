g++ fleetXpress.cpp fleet.cpp utilityFunctions.cpp dateTimeFunctions.cpp -o fleetapp -lcrypto -lsqlite3
./fleetapp
rm -f fleetapp
rm -f fleet.db

#!/bin/bash

# echo "🔧 Compiling source files to object files..."
# g++ -c -g \
#   fleetXpress.cpp \
#   fleet.cpp \
#   utilityFunctions.cpp \
#   dateTimeFunctions.cpp \
#   -lcrypto -lsqlite3

# echo -e "\n📊 --- Function Size Report ---"

# for obj in *.o; do
#   echo -e "\n== Function sizes in $obj =="
#   nm -S --size-sort --print-size "$obj" \
#     | grep ' T ' \
#     | while read addr size type name; do
#         printf "%-40s %d bytes\n" "$name" $((16#$size))
#       done
# done


# echo -e "\n🧹 Cleaning up object files..."
# rm -f *.o

# echo -e "\n✅ Done."
