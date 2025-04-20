g++ -s -shared -fPIC -m64 -O3 -o SQLiteDatabase.dll SQLiteDatabase.cpp -lsqlite3
cp SQLiteDatabase.dll ../SQLiteStatement/SQLiteDatabase.dll